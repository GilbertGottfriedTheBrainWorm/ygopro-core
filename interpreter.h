/*
 * interpreter.h
 *
 *  Created on: 2010-4-28
 *      Author: Argon
 */

#ifndef INTERPRETER_H_
#define INTERPRETER_H_

// Due to longjmp behaviour, we must build Lua as C++ to avoid UB
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "common.h"
#include <unordered_map>
#include <list>
#include <vector>
#include <utility> //std::forward
#include <cstdio>
#include <cstring>
#include <cmath>
#include <memory>
#include "lua_obj.h"

class card;
class effect;
class group;
class duel;

class Lua_Function_s {
public:
	Lua_Function_s(duel* d, int32 f) :pduel(d), function(f) {};
	int32 Value() { return function; };
	~Lua_Function_s();
private:
	duel* pduel;
	int32 function;
};

using Function = std::shared_ptr<Lua_Function_s>;
using Lua_Callable = const Function&;
/*class Lua_Callable {
public:
	Lua_Callable(int32 f) :function(f) {};
	Lua_Callable(const Function& f) :function(f->Value()) {};
	int32 Get() { return function; };
	bool Valid() { return function != 0; };
private:
	int32 function;
};*/

using lua_invalid = lua_obj_helper<PARAM_TYPE_DELETED>;

class interpreter {
	char msgbuf[128];
public:
	using coroutine_map = std::unordered_map<int32, std::pair<lua_State*, int32>>;
	using param_list = std::list<std::pair<lua_Integer, uint32>>;
	
	duel* pduel;
	lua_State* lua_state;
	lua_State* current_state;
	param_list params;
	param_list resumes;
	coroutine_map coroutines;
	int32 no_action;
	int32 call_depth;
	lua_invalid deleted;

	explicit interpreter(duel* pd);
	~interpreter();

	void register_card(card* pcard);
	void register_effect(effect* peffect);
	void unregister_effect(effect* peffect);
	void register_group(group* pgroup);
	void unregister_group(group* pgroup);
	void register_obj(lua_obj* obj, const char* tablename);

	bool load_script(const char* buffer, int len = 0, const char* script_name = nullptr);
	bool load_card_script(uint32 code);
	void add_param(void* param, int32 type, bool front = false);
	void add_param(lua_Integer  param, int32 type, bool front = false);
	void push_param(lua_State* L, bool is_coroutine = false);
	bool call_function(Lua_Callable f, uint32 param_count, int32 ret_count);
	bool call_card_function(card* pcard, const char* f, uint32 param_count, int32 ret_count, bool forced = true);
	bool call_code_function(uint32 code, const char* f, uint32 param_count, int32 ret_count);
	bool check_condition(Lua_Callable f, uint32 param_count);
	bool check_matching(card* pcard, int32 findex, int32 extraargs);
	bool check_matching_table(card* pcard, int32 findex, int32 table_index);
	int32 get_operation_value(card* pcard, int32 findex, int32 extraargs);
	bool get_operation_value(card* pcard, int32 findex, int32 extraargs, std::vector<int32>* result);
	int32 get_function_value(Lua_Callable f, uint32 param_count);
	bool get_function_value(Lua_Callable f, uint32 param_count, std::vector<int32>* result);
	int32 call_coroutine(Lua_Callable f, uint32 param_count, uint32* yield_value, uint16 step);
	void* get_ref_object(int32 ref_handler);
	bool call_function(int param_count, int ret_count);
	inline bool ret_fail(const char* message);
	inline bool ret_fail(const char* message, bool error);
	inline void deepen();
	inline void flatten();

	static void pushobject(lua_State* L, lua_obj* obj);
	static void pushobject(lua_State* L, const Function& obj);
	static void pushobject(lua_State* L, int32 lua_ptr);
	static int pushExpandedTable(lua_State* L, int32 table_index);
	static inline duel* get_duel_info(lua_State* L) {
		duel* pduel;
		memcpy(&pduel, lua_getextraspace(L), LUA_EXTRASPACE);
		return pduel;
	}
	static void print_stacktrace(lua_State* L);

	template <typename... TR>
	inline const char* format(const char* format, TR&&... args) {
		if(std::snprintf(msgbuf, sizeof(msgbuf), format, std::forward<TR>(args)...) >= 0)
			return msgbuf;
		return "";
	}
};

#define COROUTINE_FINISH	1
#define COROUTINE_YIELD		2
#define COROUTINE_ERROR		3

static_assert(LUA_VERSION_NUM == 503 || LUA_VERSION_NUM == 504, "Lua 5.3 or 5.4 is required, the core won't work with other lua versions");
static_assert(LUA_MAXINTEGER >= INT64_MAX, "Lua has to support 64 bit integers");
#if LUA_VERSION_NUM <= 503
#define lua_resumec(state, from, nargs, res) lua_resume(state, from, nargs)
#else
#define lua_resumec(state, from, nargs, res) lua_resume(state, from, nargs, res)
#endif

#endif /* INTERPRETER_H_ */
