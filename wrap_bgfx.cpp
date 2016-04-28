extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
#include <bgfx/c99/bgfx.h>
}

#include <map>

namespace {
	std::map<const char*, uint32_t> debug_lookup = {
		{ "wireframe", BGFX_DEBUG_WIREFRAME },
		{ "stats",     BGFX_DEBUG_STATS },
		{ "ifh",       BGFX_DEBUG_IFH },
		{ "text",      BGFX_DEBUG_TEXT }
	};

	std::map<const char*, uint32_t> reset_lookup = {
		{ "vsync",              BGFX_RESET_VSYNC },
		{ "depth_clamp",        BGFX_RESET_DEPTH_CLAMP },
		{ "srgb_backbuffer",    BGFX_RESET_SRGB_BACKBUFFER },
		{ "flip_after_render",  BGFX_RESET_FLIP_AFTER_RENDER },
		{ "flush_after_render", BGFX_RESET_FLUSH_AFTER_RENDER }
	};

	template <typename F>
	void table_scan(lua_State *L, int index, F fn) {
		lua_pushvalue(L, index);
		lua_pushnil(L);

		while (lua_next(L, -2)) {
			lua_pushvalue(L, -2);
			const char *key = lua_tostring(L, -1);
			const char *value = lua_tostring(L, -2);
			lua_pop(L, 2);

			if (!fn(key, value)) {
				break;
			}
		}

		lua_pop(L, 1);
	}

	const luaL_Reg m[] = {
		// TODO: actually take some args for this
		{ "init", [](lua_State *) {
			bgfx_init(BGFX_RENDERER_TYPE_OPENGL, BGFX_PCI_ID_NONE, 0, NULL, NULL);
			return 0;
		} },

		{ "shutdown", [](lua_State *) {
			bgfx_shutdown();
			return 0;
		} },

		{ "dbg_text_print", [](lua_State *L) {
			int n = lua_gettop(L);
			lua_assert(n == 4);
			(void)n;
			uint16_t x = (uint16_t)lua_tonumber(L, -1);
			uint16_t y = (uint16_t)lua_tonumber(L, -2);
			uint8_t attr = (uint8_t)lua_tonumber(L, -3);
			const char *str = lua_tostring(L, -4);
			bgfx_dbg_text_printf(x, y, attr, "%s", str);
			return 0;
		} },

		{ "dbg_text_clear", [](lua_State *L) {
			int n = lua_gettop(L);
			lua_assert(n >= 0 || n <= 2);
			uint8_t attr = n >= 1 ? (uint8_t)lua_tonumber(L, -1) : 0;
			bool small   = n >= 2 ? (bool)lua_toboolean(L, -2) : false;
			bgfx_dbg_text_clear(attr, small);
			return 0;
		} },

		// bgfx.set_debug {
		// 	"wireframe",
		// 	"stats",
		// 	"ifh",
		// 	"text"
		// }
		{ "set_debug", [](lua_State *L) {
			uint32_t debug = 0;

			table_scan(L, -1, [&](const char *, const char *v) {
				auto val = debug_lookup.find(v);
				if (val != debug_lookup.end()) {
					debug |= val->second;
				}
				return true;
			});

			bgfx_set_debug(debug);

			return 0;
		} },

		// bgfx.reset (1280, 720 {
		// 	"vsync",
		// 	"depth_clamp",
		// 	"srgb_backbuffer",
		// 	"flip_after_render",
		// 	"flush_after_render"
		// })

		{ "reset", [](lua_State *L) {
			int n = lua_gettop(L);
			lua_assert(n == 3);
			(void)n;
			int w = (int)lua_tonumber(L, -1);
			int h = (int)lua_tonumber(L, -2);
			uint32_t reset = 0;

			table_scan(L, -3, [&](const char *, const char *v) {
				auto val = reset_lookup.find(v);
				if (val != reset_lookup.end()) {
					reset |= val->second;
				}
				return true;
			});

			bgfx_reset(w, h, reset);
			return 0;
		} },

		{ "touch", [](lua_State *L) {
			int n = lua_gettop(L);
			lua_assert(n == 1);
			(void)n;
			uint8_t id = (uint8_t)lua_tonumber(L, -1);
			unsigned int ret = bgfx_touch(id);
			lua_pushnumber(L, double(ret));
			return 1;
		} },

		{ "frame", [](lua_State *L) {
			int r = bgfx_frame();
			lua_pushnumber(L, (double)r);
			return 1;
		} },

		// TODO: parse stuff
		{ "set_state", [](lua_State *) {
			uint64_t flags = 0
				| BGFX_STATE_MSAA
				| BGFX_STATE_ALPHA_WRITE
				| BGFX_STATE_RGB_WRITE
				| BGFX_STATE_CULL_CCW
				| BGFX_STATE_DEPTH_WRITE
				| BGFX_STATE_DEPTH_TEST_LEQUAL
			;
			uint32_t rgba = 0;
			bgfx_set_state((uint32_t)flags, rgba);
			return 0;
		} },

		{ "set_view_rect", [](lua_State *L) {
			int n = lua_gettop(L);
			lua_assert(n == 5);
			(void)n;
			uint8_t id = (uint8_t)lua_tonumber(L, -1);
			uint16_t x = (uint16_t)lua_tonumber(L, -2);
			uint16_t y = (uint16_t)lua_tonumber(L, -3);
			uint16_t w = (uint16_t)lua_tonumber(L, -4);
			uint16_t h = (uint16_t)lua_tonumber(L, -5);
			bgfx_set_view_rect(id, x, y, w, h);
			return 0;
		} },

		{ "set_view_rect_auto", [](lua_State *L) {
			uint8_t id = (uint8_t)lua_tonumber(L, -1);
			uint16_t x = (uint16_t)lua_tonumber(L, -2);
			uint16_t y = (uint16_t)lua_tonumber(L, -3);
			bgfx_set_view_rect_auto(id, x, y, BGFX_BACKBUFFER_RATIO_EQUAL);
			return 0;
		} },

		{ "set_view_scissor", [](lua_State *L) {
			int n = lua_gettop(L);
			lua_assert(n == 5);
			(void)n;
			uint8_t id = (uint8_t)lua_tonumber(L, -1);
			uint16_t x = (uint16_t)lua_tonumber(L, -2);
			uint16_t y = (uint16_t)lua_tonumber(L, -3);
			uint16_t w = (uint16_t)lua_tonumber(L, -4);
			uint16_t h = (uint16_t)lua_tonumber(L, -5);
			bgfx_set_view_scissor(id, x, y, w, h);
			return 0;
		} },

		{ "set_view_clear", [](lua_State *L) {
			int n = lua_gettop(L);
			lua_assert(n == 5);
			(void)n;
			uint8_t id = (uint8_t)lua_tonumber(L, -1);
			uint16_t flags = (uint16_t)lua_tonumber(L, -2);
			uint32_t rgba = (uint32_t)lua_tonumber(L, -3);
			float depth = (float)lua_tonumber(L, -4);
			uint8_t stencil = (uint8_t)lua_tonumber(L, -5);
			bgfx_set_view_clear(id, flags, rgba, depth, stencil);
			return 0;
		} },

		{ "set_view_name", [](lua_State *L) {
			int n = lua_gettop(L);
			lua_assert(n == 2);
			(void)n;
			uint8_t id = (uint8_t)lua_tonumber(L, -1);
			const char *name = lua_tostring(L, -2);
			bgfx_set_view_name(id, name);
			return 0;
		} },

		{ "submit", [](lua_State *L) {
			int n = lua_gettop(L);
			lua_assert(n == 4);
			(void)n;
			uint8_t id = (uint8_t)lua_tonumber(L, -1);
			bgfx_program_handle_t program = {};
			bool depth = 1.0;
			bool preserve_state = false;
			uint32_t r = bgfx_submit(id, program, depth, preserve_state);
			lua_pushnumber(L, r);
			return 1;
		} },

		{ "set_view_transform", [](lua_State *) {
			// bgfx_set_view_transform(id, view, proj);
			// bgfx_set_view_transform_stereo(id, view, proj_l, flags, proj_r);
			return 0;
		} },

		{ "set_transform", [](lua_State *) {
			// bgfx_set_transform(mtx, num);
			return 0;
		} },

		{ "set_vertex_buffer", [](lua_State *) {
			// bgfx_set_vertex_buffer(handle, start, num);
			return 0;
		} },

		{ "set_index_buffer", [](lua_State *) {
			// bgfx_set_index_buffer(handle, first, num);
			return 0;
		} },

		{ "get_hmd", [](lua_State *) {
			// TODO: turn this thing into a table
			const bgfx_hmd_t *hmd = bgfx_get_hmd();
			(void)hmd;
			return 0;
		} },

		{ NULL, NULL }
	};

	struct shader_ud_t {
		bgfx_shader_handle_t handle;
	};

	shader_ud_t *to_shader_ud(lua_State *L, int index) {
		shader_ud_t **ud = (shader_ud_t**)lua_touserdata(L, index);
		if (ud == NULL) luaL_typerror(L, index, "bgfx_shader");
		return *ud;
	}

	const luaL_Reg shader_fn[] = {
		{ "new", [](lua_State *L) {
			shader_ud_t **ud = (shader_ud_t**)lua_newuserdata(L, sizeof(shader_ud_t*));
			*ud = new shader_ud_t();

			const void *data = lua_topointer(L, -1);
			unsigned int size = (unsigned int)lua_tonumber(L, -2);
			lua_assert(data != NULL);
			const bgfx_memory_t *mem = bgfx_make_ref(data, size);
			(*ud)->handle = bgfx_create_shader(mem);

			luaL_getmetatable(L, "bgfx_shader");
			lua_setmetatable(L, -2);
			return 1;
		} },
		{ "__gc", [](lua_State *L) {
			shader_ud_t *ud = to_shader_ud(L, 1);
			bgfx_destroy_shader(ud->handle);
			return 0;
		} },
		{ "__tostring", [](lua_State *L) {
			char buff[32];
			sprintf(buff, "%p", to_shader_ud(L, 1));
			lua_pushfstring(L, "bgfx_shader (%s)", buff);
			return 0;
		} },
		{ NULL, NULL }
	};

	struct program_ud_t {
		bgfx_program_handle_t handle;
	};

	program_ud_t *to_program_ud(lua_State *L, int index) {
		program_ud_t **ud = (program_ud_t**)lua_touserdata(L, index);
		if (ud == NULL) luaL_typerror(L, index, "bgfx_program");
		return *ud;
	}

	const luaL_Reg program_fn[] = {
		{ "new", [](lua_State *L) {
			program_ud_t **ud = (program_ud_t**)lua_newuserdata(L, sizeof(program_ud_t*));
			*ud = new program_ud_t();

			// lua_assert(data != NULL);
			shader_ud_t *vsh = to_shader_ud(L, -1);
			shader_ud_t *fsh = to_shader_ud(L, -1);

			(*ud)->handle = bgfx_create_program(vsh->handle, fsh->handle, false);

			luaL_getmetatable(L, "bgfx_program");
			lua_setmetatable(L, -2);
			return 1;
		} },
		{ "__gc",  [](lua_State *L) {
			program_ud_t *ud = to_program_ud(L, 1);
			bgfx_destroy_program(ud->handle);
			return 0;
		} },
		{ "__tostring", [](lua_State *L) {
			char buff[32];
			sprintf(buff, "%p", to_program_ud(L, 1));
			lua_pushfstring(L, "bgfx_program (%s)", buff);
			return 0;
		} },
		{ NULL, NULL }
	};

	struct vertex_buffer_ud_t {
		bgfx_vertex_buffer_handle_t handle;
	};

	vertex_buffer_ud_t *to_vertex_buffer_ud(lua_State *L, int index) {
		vertex_buffer_ud_t **ud = (vertex_buffer_ud_t**)lua_touserdata(L, index);
		if (ud == NULL) luaL_typerror(L, index, "bgfx_vertex_buffer");
		return *ud;
	}

	const luaL_Reg vertex_buffer_fn[] = {
		{ "new", [](lua_State *L) {
			vertex_buffer_ud_t **ud = (vertex_buffer_ud_t**)lua_newuserdata(L, sizeof(vertex_buffer_ud_t*));
			*ud = new vertex_buffer_ud_t();

			// lua_assert(data != NULL);

			const bgfx_memory_t *mem = NULL;
			uint16_t flags = 0;
			bgfx_vertex_decl_t *decl = NULL;

			(*ud)->handle = bgfx_create_vertex_buffer(mem, decl, flags);

			luaL_getmetatable(L, "bgfx_vertex_buffer");
			lua_setmetatable(L, -2);
			return 1;
		} },
		{ "__gc",  [](lua_State *L) {
			vertex_buffer_ud_t *ud = to_vertex_buffer_ud(L, 1);
			bgfx_destroy_vertex_buffer(ud->handle);
			return 0;
		} },
		{ "__tostring", [](lua_State *L) {
			char buff[32];
			sprintf(buff, "%p", to_vertex_buffer_ud(L, 1));
			lua_pushfstring(L, "bgfx_vertex_buffer (%s)", buff);
			return 0;
		} },
		{ NULL, NULL }
	};

	struct index_buffer_ud_t {
		bgfx_index_buffer_handle_t handle;
	};

	index_buffer_ud_t *to_index_buffer_ud(lua_State *L, int index) {
		index_buffer_ud_t **ud = (index_buffer_ud_t**)lua_touserdata(L, index);
		if (ud == NULL) luaL_typerror(L, index, "bgfx_index_buffer");
		return *ud;
	}

	const luaL_Reg index_buffer_fn[] = {
		{ "new", [](lua_State *L) {
			index_buffer_ud_t **ud = (index_buffer_ud_t**)lua_newuserdata(L, sizeof(index_buffer_ud_t*));
			*ud = new index_buffer_ud_t();

			lua_assert(data != NULL);
			const bgfx_memory_t *mem = NULL;
			uint16_t flags = 0;

			(*ud)->handle = bgfx_create_index_buffer(mem, flags);

			luaL_getmetatable(L, "bgfx_index_buffer");
			lua_setmetatable(L, -2);
			return 1;
		} },
		{ "__gc",  [](lua_State *L) {
			index_buffer_ud_t *ud = to_index_buffer_ud(L, 1);
			bgfx_destroy_index_buffer(ud->handle);
			return 0;
		} },
		{ "__tostring", [](lua_State *L) {
			char buff[32];
			sprintf(buff, "%p", to_index_buffer_ud(L, 1));
			lua_pushfstring(L, "bgfx_index_buffer (%s)", buff);
			return 0;
		} },
		{ NULL, NULL }
	};

}

#ifdef _MSC_VER
# define LUA_EXPORT __declspec(dllexport)
#else
# define LUA_EXPORT
#endif

extern "C" LUA_EXPORT int luaopen_bgfx(lua_State*);

LUA_EXPORT
int luaopen_bgfx(lua_State *L) {
	luaL_newmetatable(L, "bgfx_shader");
	luaL_register(L, NULL, shader_fn);
	lua_pushvalue(L, -1);
	lua_setfield(L, -1, "__index");

	luaL_newmetatable(L, "bgfx_program");
	luaL_register(L, NULL, program_fn);
	lua_pushvalue(L, -1);
	lua_setfield(L, -1, "__index");

	luaL_newmetatable(L, "bgfx_vertex_buffer");
	luaL_register(L, NULL, vertex_buffer_fn);
	lua_pushvalue(L, -1);
	lua_setfield(L, -1, "__index");

	luaL_newmetatable(L, "bgfx_index_buffer");
	luaL_register(L, NULL, index_buffer_fn);
	lua_pushvalue(L, -1);
	lua_setfield(L, -1, "__index");

	luaL_register(L, "bgfx", m);

	return 1;
}
