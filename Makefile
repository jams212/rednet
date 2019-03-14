include platform.mk

BUILD_PATH := ./build

STD_DEFINE := -std=c++11
INCDIR := ./rednet-src ./3rd ./3rd/jemalloc/include/jemalloc ./3rd/lua/src /usr/local/include/jemalloc
LIBDIR := $(CXX_LIB_PATH) $(CXX_SERVICE_PATH) $(CXX_3RD_PATH) /usr/local/lib /usr/lib

CXX_FLAGS = -g -O2 -Wall $(STD_DEFINE) $(foreach v, $(INCDIR), -I$(v)) $(foreach v, $(LIBDIR), -L$(v)) $(MYCFLAGS)

CXX_SERVICE := logger script netgames
PROJECT_NAME := rednet 

# lua
LUA_STATIC_LIB := 3rd_cxx/liblua.a

# jemalloc
JEMALLOC_SHARED_LIB := 3rd_cxx/libjemalloc.so
MALLOC_SHAREDLIB := $(JEMALLOC_SHARED_LIB)


SRCS = umemory_hook.cc \
	   uglobal.cc \
	   usignal.cc \
	   uevent_queue.cc \
	   ucontext.cc \
	   ucomponent.cc \
	   umodule.cc \
	   udaemon.cc \
	   ugroup.cc \
	   utimer.cc \
	   uwork.cc \
	   ucall.cc \
	   uini.cc \
	   uenv.cc \
	   uframework.cc \
	   unetdrive.cc \
	   lua/uluaseri.cc \
	   network/unetwork.cc \
	   network/uepoll.cc \
	   network/ukqueue.cc \
	   network/uchannel.cc \
	   network/usocket_server.cc \
	   network/usocket.cc \
	\

LUA_LIB_REDNET = lua-core.cc\
\

all :\
	$(CXX_LIB_PATH)/librednet.so \
	$(foreach v, $(CXX_SERVICE), $(CXX_SERVICE_PATH)/$(v).so) \
	$(BUILD_PATH)/$(PROJECT_NAME) \
	$(CXX_LIB_PATH)/rednet.so 
	
$(LUA_STATIC_LIB) :	| $(CXX_3RD_PATH)
	cd 3rd/lua && $(MAKE) CC='$(CC) -std=gnu99 -fPIC' $(PLAT) && mv ./src/liblua.a ../../3rd_cxx/liblua.a

$(MALLOC_SHAREDLIB) : 3rd/jemalloc/Makefile
	cd 3rd/jemalloc && sudo $(MAKE) CC=$(CC) && sudo mv ./lib/libjemalloc.so ../../3rd_cxx/libjemalloc.so


3rd/jemalloc/Makefile : 
	cd 3rd/jemalloc && ./autogen.sh --with-jemalloc-prefix=je_ --disable-valgrind

$(CXX_LIB_PATH)/librednet.so : $(foreach v, $(SRCS), rednet-src/$(v)) $(LUA_STATIC_LIB) | $(CXX_LIB_PATH)
	$(CXX) $(CXX_FLAGS) $(SHARED) -o $@ $^ $(LDFLAGS) $(BOOST_LIBS) $(CXX_LIBS) $(BOOST_DEFINES) $(LIB_DEFINES) $(EXPORT)

$(BUILD_PATH)/$(PROJECT_NAME) : rednet-src/main.cc | $(BUILD_PATH)
	$(CXX) $(CXX_FLAGS) -o $@ $^ -lrednet $(RPATH)

$(CXX_LIB_PATH)/rednet.so : $(addprefix lualib-src/,$(LUA_LIB_REDNET)) | $(CXX_LIB_PATH)
	$(CXX) $(CXX_FLAGS) $(SHARED) -o $@ $^ -lrednet $(RPATH) -Irednet-src -Ilualib-src -Iservice-src

$(CXX_LIB_PATH) :
	mkdir $(CXX_LIB_PATH)

$(CXX_SERVICE_PATH) :
	mkdir $(CXX_SERVICE_PATH)

$(CXX_3RD_PATH) :
	mkdir $(CXX_3RD_PATH)

$(BUILD_PATH) :
	mkdir $(BUILD_PATH)

define CXX_SERVICE_TEMP
$$(CXX_SERVICE_PATH)/$(1).so : service-src/service_$(1).cc  | $$(CXX_SERVICE_PATH)
	$$(CXX) $$(CXX_FLAGS) $$(SHARED) $$< -o $$@ -Irednet-src -lrednet $$(LUA_STATIC_LIB) $$(RPATH)
endef

$(foreach v, $(CXX_SERVICE), $(eval $(call CXX_SERVICE_TEMP,$(v))))

clean:
	rm -f $(CXX_LIB_PATH)/*.so $(CXX_SERVICE_PATH)/*.so $(CXX_3RD_PATH)/*.so $(PROJECT_NAME)

cleanall: clean 
ifneq (,$(wildcard 3rd/jemalloc/Makefile))
	cd 3rd/jemalloc && $(MAKE) clean && rm Makefile
endif
	cd 3rd/aoi && $(MAKE) clean