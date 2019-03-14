CC := gcc
CXX := g++

PLAT := none

.PHONY : none $(PLAT) clean all cleanall

.PHONY : default

default :
	$(MAKE) $(PLAT)

none :
	@echo "Please do 'make PLATFORM' where PLATFORM is one of these:"
	@echo " $(PLAT)"

linux : PLAT = linux
macosx : PLAT = macosx

SHARED := -fPIC --shared
EXPORT :=

CXX_LIB_PATH := lib_cxx
CXX_SERVICE_PATH := service_cxx
CXX_3RD_PATH := 3rd_cxx

BOOST_LIBS := 
CXX_LIBS := -lpthread

RPATH := -Wl,-rpath=./$(CXX_LIB_PATH):../$(CXX_LIB_PATH):./$(3rd_cxx):../$(3rd_cxx):/usr/local/lib


macosx : SHARED := -fPIC -dynamiclib -Wl,-undefined,dynamic_lookup
macosx : EXPORT := 
linux  : CXX_LIBS += -ljemalloc
macosx linux : CXX_LIBS += -ldl

macosx : BOOST_DEFINES := 
macosx : LIB_DEFINES := -DNOUSE_JEMALLOC


linux : BOOST_DEFINES := 
linux : LIB_DEFINES := -DUSE_JEMALLOC -DJEMALLOC_NO_DEMANGLE


linux macosx :
	$(MAKE) all PLAT=$@ BOOST_LIBS="$(BOOST_LIBS)" CXX_LIBS="$(CXX_LIBS)" SHARED="$(SHARED)" EXPORT="$(EXPORT)" RPATH="$(RPATH)" LIB_DEFINES="$(LIB_DEFINES)" BOOST_DEFINES="$(BOOST_DEFINES)"