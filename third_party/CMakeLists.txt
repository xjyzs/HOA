cmake_minimum_required(VERSION 3.10)
project(hongengine_libs LANGUAGES C CXX ASM)

# ----- Paths -----
set(SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/arkcompiler_runtime_core/static_core)
set(GEN_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/build_gen)
set(THIRD_PARTY ${SRC_ROOT}/third_party)
set(ETS_PLUGIN_ROOT ${SRC_ROOT}/plugins/ets)

# ----- C++ standard -----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ----- Output directories -----
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ----- Compile flags (matching original Panda build, minus -Werror for NDK compat) -----
add_compile_options(-fno-rtti -fno-exceptions)
add_compile_options(-pedantic -Wall -Wextra -Wshadow)

# Android NDK may warn more aggressively; demote some
add_compile_options(-Wno-unused-parameter -Wno-deprecated-declarations)

# Platform defines for Android/arm64
add_compile_definitions(
    PANDA_TARGET_MOBILE
    PANDA_TARGET_UNIX
    PANDA_TARGET_ARM64
    PANDA_TARGET_64
    PANDA_USE_FUTEX
    PANDA_USE_32_BIT_POINTER
    PANDA_CMAKE_SDK
    PANDA_PRODUCT_BUILD
    PANDA_BUILD
    PANDA_WITH_CODEGEN
    PANDA_COMPILER_TARGET_AARCH64
)

# Linker flags for mobile arm64
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fuse-ld=lld")

# ----- Global include directories -----
include_directories(
    ${SRC_ROOT}
    ${THIRD_PARTY}/utils_native/base/include
    ${THIRD_PARTY}/zlib
    ${GEN_ROOT}/generated
    ${GEN_ROOT}/panda_gen_options
    ${GEN_ROOT}/libpandabase/generated
    ${GEN_ROOT}/libpandafile/include
    ${GEN_ROOT}/compiler
    ${GEN_ROOT}
    ${GEN_ROOT}/runtime/asm_defines
    ${GEN_ROOT}/runtime/include
    ${GEN_ROOT}/cross_values
)

# ==============================================================================
# 1. libsecurec (c_secshared) - secure C runtime
# ==============================================================================
set(SECUREC_ROOT ${THIRD_PARTY}/utils_native/base)

set(SECUREC_SOURCES
    ${SECUREC_ROOT}/src/securec/vsprintf_s.c
    ${SECUREC_ROOT}/src/securec/wmemmove_s.c
    ${SECUREC_ROOT}/src/securec/strncat_s.c
    ${SECUREC_ROOT}/src/securec/vsnprintf_s.c
    ${SECUREC_ROOT}/src/securec/fwscanf_s.c
    ${SECUREC_ROOT}/src/securec/scanf_s.c
    ${SECUREC_ROOT}/src/securec/strcat_s.c
    ${SECUREC_ROOT}/src/securec/sscanf_s.c
    ${SECUREC_ROOT}/src/securec/secureprintoutput_w.c
    ${SECUREC_ROOT}/src/securec/wmemcpy_s.c
    ${SECUREC_ROOT}/src/securec/wcsncat_s.c
    ${SECUREC_ROOT}/src/securec/secureprintoutput_a.c
    ${SECUREC_ROOT}/src/securec/secureinput_w.c
    ${SECUREC_ROOT}/src/securec/memcpy_s.c
    ${SECUREC_ROOT}/src/securec/fscanf_s.c
    ${SECUREC_ROOT}/src/securec/vswscanf_s.c
    ${SECUREC_ROOT}/src/securec/secureinput_a.c
    ${SECUREC_ROOT}/src/securec/sprintf_s.c
    ${SECUREC_ROOT}/src/securec/memmove_s.c
    ${SECUREC_ROOT}/src/securec/swscanf_s.c
    ${SECUREC_ROOT}/src/securec/snprintf_s.c
    ${SECUREC_ROOT}/src/securec/vscanf_s.c
    ${SECUREC_ROOT}/src/securec/vswprintf_s.c
    ${SECUREC_ROOT}/src/securec/wcscpy_s.c
    ${SECUREC_ROOT}/src/securec/vfwscanf_s.c
    ${SECUREC_ROOT}/src/securec/memset_s.c
    ${SECUREC_ROOT}/src/securec/wscanf_s.c
    ${SECUREC_ROOT}/src/securec/vwscanf_s.c
    ${SECUREC_ROOT}/src/securec/strtok_s.c
    ${SECUREC_ROOT}/src/securec/wcsncpy_s.c
    ${SECUREC_ROOT}/src/securec/vfscanf_s.c
    ${SECUREC_ROOT}/src/securec/vsscanf_s.c
    ${SECUREC_ROOT}/src/securec/wcstok_s.c
    ${SECUREC_ROOT}/src/securec/securecutil.c
    ${SECUREC_ROOT}/src/securec/gets_s.c
    ${SECUREC_ROOT}/src/securec/swprintf_s.c
    ${SECUREC_ROOT}/src/securec/strcpy_s.c
    ${SECUREC_ROOT}/src/securec/wcscat_s.c
    ${SECUREC_ROOT}/src/securec/strncpy_s.c
)

add_library(c_secshared SHARED ${SECUREC_SOURCES})
target_include_directories(c_secshared PUBLIC ${SECUREC_ROOT}/include)
target_compile_options(c_secshared PRIVATE -Wno-all -Wno-extra)

# ==============================================================================
# 2. libz (zlib) - compression
# ==============================================================================
set(ZLIB_ROOT ${THIRD_PARTY}/zlib)

set(ZLIB_SOURCES
    ${ZLIB_ROOT}/adler32.c
    ${ZLIB_ROOT}/compress.c
    ${ZLIB_ROOT}/contrib/minizip/ioapi.c
    ${ZLIB_ROOT}/contrib/minizip/unzip.c
    ${ZLIB_ROOT}/contrib/minizip/zip.c
    ${ZLIB_ROOT}/crc32.c
    ${ZLIB_ROOT}/deflate.c
    ${ZLIB_ROOT}/gzclose.c
    ${ZLIB_ROOT}/gzlib.c
    ${ZLIB_ROOT}/gzread.c
    ${ZLIB_ROOT}/gzwrite.c
    ${ZLIB_ROOT}/inflate.c
    ${ZLIB_ROOT}/infback.c
    ${ZLIB_ROOT}/inftrees.c
    ${ZLIB_ROOT}/inffast.c
    ${ZLIB_ROOT}/trees.c
    ${ZLIB_ROOT}/uncompr.c
    ${ZLIB_ROOT}/zutil.c
)

add_library(z SHARED ${ZLIB_SOURCES})
target_include_directories(z PUBLIC ${ZLIB_ROOT} ${ZLIB_ROOT}/contrib/minizip)
target_compile_definitions(z PRIVATE HAVE_UNISTD_H=1 Z_HAVE_UNISTD_H=1)

# ==============================================================================
# 3. ICU 72 STATIC libraries — stubdata, hmicuuc.z, hmicui18n.z
# Sources extracted from static_core/cmake/third_party/icu/CMakeLists.txt
# ==============================================================================
set(ICU_ROOT ${THIRD_PARTY}/icu)

# --- stubdata ---
add_library(stubdata STATIC
    ${ICU_ROOT}/icu4c/source/stubdata/stubdata.cpp
)

target_include_directories(stubdata
    SYSTEM PUBLIC ${ICU_ROOT}/icu4c/source/common
    PRIVATE ${ICU_ROOT}/icu4c/source
)

target_compile_definitions(stubdata PRIVATE
    -DU_ATTRIBUTE_DEPRECATED=
    -DU_COMMON_IMPLEMENTATION
    -DU_STATIC_IMPLEMENTATION
    -DUPRV_BLOCK_MACRO_BEGIN=
    -DUPRV_BLOCK_MACRO_END=
    -DUCONFIG_USE_WINDOWS_LCID_MAPPING_API=0
    -D_REENTRANT
)

target_compile_options(stubdata PRIVATE
    -std=c++11 -frtti -fexceptions -Wno-shadow -Wno-unused
)

# --- hmicuuc.z (common) ---
set(ICUUC_SOURCES
    ${ICU_ROOT}/icu4c/source/common/unistr_case.cpp
    ${ICU_ROOT}/icu4c/source/common/loclikely.cpp
    ${ICU_ROOT}/icu4c/source/common/loclikelysubtags.cpp
    ${ICU_ROOT}/icu4c/source/common/ustrcase_locale.cpp
    ${ICU_ROOT}/icu4c/source/common/usprep.cpp
    ${ICU_ROOT}/icu4c/source/common/uhash_us.cpp
    ${ICU_ROOT}/icu4c/source/common/ucharstriebuilder.cpp
    ${ICU_ROOT}/icu4c/source/common/uloc.cpp
    ${ICU_ROOT}/icu4c/source/common/rbbi.cpp
    ${ICU_ROOT}/icu4c/source/common/locresdata.cpp
    ${ICU_ROOT}/icu4c/source/common/unistr_cnv.cpp
    ${ICU_ROOT}/icu4c/source/common/unifiedcache.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnvlat1.cpp
    ${ICU_ROOT}/icu4c/source/common/lsr.cpp
    ${ICU_ROOT}/icu4c/source/common/ustack.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnv2022.cpp
    ${ICU_ROOT}/icu4c/source/common/ustrenum.cpp
    ${ICU_ROOT}/icu4c/source/common/uresbund.cpp
    ${ICU_ROOT}/icu4c/source/common/ucharstrieiterator.cpp
    ${ICU_ROOT}/icu4c/source/common/unormcmp.cpp
    ${ICU_ROOT}/icu4c/source/common/util.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnv_lmb.cpp
    ${ICU_ROOT}/icu4c/source/common/uscript.cpp
    ${ICU_ROOT}/icu4c/source/common/unames.cpp
    ${ICU_ROOT}/icu4c/source/common/ubiditransform.cpp
    ${ICU_ROOT}/icu4c/source/common/localematcher.cpp
    ${ICU_ROOT}/icu4c/source/common/rbbiscan.cpp
    ${ICU_ROOT}/icu4c/source/common/errorcode.cpp
    ${ICU_ROOT}/icu4c/source/common/uloc_tag.cpp
    ${ICU_ROOT}/icu4c/source/common/unisetspan.cpp
    ${ICU_ROOT}/icu4c/source/common/ustr_cnv.cpp
    ${ICU_ROOT}/icu4c/source/common/patternprops.cpp
    ${ICU_ROOT}/icu4c/source/common/rbbinode.cpp
    ${ICU_ROOT}/icu4c/source/common/utrace.cpp
    ${ICU_ROOT}/icu4c/source/common/characterproperties.cpp
    ${ICU_ROOT}/icu4c/source/common/loadednormalizer2impl.cpp
    ${ICU_ROOT}/icu4c/source/common/resource.cpp
    ${ICU_ROOT}/icu4c/source/common/resbund.cpp
    ${ICU_ROOT}/icu4c/source/common/bytesinkutil.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnv_err.cpp
    ${ICU_ROOT}/icu4c/source/common/propsvec.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnvisci.cpp
    ${ICU_ROOT}/icu4c/source/common/utrie.cpp
    ${ICU_ROOT}/icu4c/source/common/appendable.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnv_bld.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnv_u32.cpp
    ${ICU_ROOT}/icu4c/source/common/resbund_cnv.cpp
    ${ICU_ROOT}/icu4c/source/common/ucat.cpp
    ${ICU_ROOT}/icu4c/source/common/unistr.cpp
    ${ICU_ROOT}/icu4c/source/common/filteredbrk.cpp
    ${ICU_ROOT}/icu4c/source/common/parsepos.cpp
    ${ICU_ROOT}/icu4c/source/common/punycode.cpp
    ${ICU_ROOT}/icu4c/source/common/usc_impl.cpp
    ${ICU_ROOT}/icu4c/source/common/simpleformatter.cpp
    ${ICU_ROOT}/icu4c/source/common/uarrsort.cpp
    ${ICU_ROOT}/icu4c/source/common/dictionarydata.cpp
    ${ICU_ROOT}/icu4c/source/common/normalizer2.cpp
    ${ICU_ROOT}/icu4c/source/common/dictbe.cpp
    ${ICU_ROOT}/icu4c/source/common/schriter.cpp
    ${ICU_ROOT}/icu4c/source/common/bytestream.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnvscsu.cpp
    ${ICU_ROOT}/icu4c/source/common/uinit.cpp
    ${ICU_ROOT}/icu4c/source/common/ustring.cpp
    ${ICU_ROOT}/icu4c/source/common/usetiter.cpp
    ${ICU_ROOT}/icu4c/source/common/uiter.cpp
    ${ICU_ROOT}/icu4c/source/common/uhash.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnvdisp.cpp
    ${ICU_ROOT}/icu4c/source/common/icuplug.cpp
    ${ICU_ROOT}/icu4c/source/common/uniset_props.cpp
    ${ICU_ROOT}/icu4c/source/common/uvector.cpp
    ${ICU_ROOT}/icu4c/source/common/uset.cpp
    ${ICU_ROOT}/icu4c/source/common/unifunct.cpp
    ${ICU_ROOT}/icu4c/source/common/normlzr.cpp
    ${ICU_ROOT}/icu4c/source/common/uresdata.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnvhz.cpp
    ${ICU_ROOT}/icu4c/source/common/normalizer2impl.cpp
    ${ICU_ROOT}/icu4c/source/common/umutablecptrie.cpp
    ${ICU_ROOT}/icu4c/source/common/ucharstrie.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnv_u8.cpp
    ${ICU_ROOT}/icu4c/source/common/ulist.cpp
    ${ICU_ROOT}/icu4c/source/common/dtintrv.cpp
    ${ICU_ROOT}/icu4c/source/common/sharedobject.cpp
    ${ICU_ROOT}/icu4c/source/common/uscript_props.cpp
    ${ICU_ROOT}/icu4c/source/common/bytestrieiterator.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnv_cnv.cpp
    ${ICU_ROOT}/icu4c/source/common/uchar.cpp
    ${ICU_ROOT}/icu4c/source/common/ubidiln.cpp
    ${ICU_ROOT}/icu4c/source/common/icudataver.cpp
    ${ICU_ROOT}/icu4c/source/common/locutil.cpp
    ${ICU_ROOT}/icu4c/source/common/ubrk.cpp
    ${ICU_ROOT}/icu4c/source/common/propname.cpp
    ${ICU_ROOT}/icu4c/source/common/umutex.cpp
    ${ICU_ROOT}/icu4c/source/common/uenum.cpp
    ${ICU_ROOT}/icu4c/source/common/uinvchar.cpp
    ${ICU_ROOT}/icu4c/source/common/udatamem.cpp
    ${ICU_ROOT}/icu4c/source/common/locavailable.cpp
    ${ICU_ROOT}/icu4c/source/common/rbbirb.cpp
    ${ICU_ROOT}/icu4c/source/common/uobject.cpp
    ${ICU_ROOT}/icu4c/source/common/servls.cpp
    ${ICU_ROOT}/icu4c/source/common/serv.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnv_cb.cpp
    ${ICU_ROOT}/icu4c/source/common/chariter.cpp
    ${ICU_ROOT}/icu4c/source/common/edits.cpp
    ${ICU_ROOT}/icu4c/source/common/stringtriebuilder.cpp
    ${ICU_ROOT}/icu4c/source/common/utext.cpp
    ${ICU_ROOT}/icu4c/source/common/uts46.cpp
    ${ICU_ROOT}/icu4c/source/common/locdspnm.cpp
    ${ICU_ROOT}/icu4c/source/common/util_props.cpp
    ${ICU_ROOT}/icu4c/source/common/utypes.cpp
    ${ICU_ROOT}/icu4c/source/common/unistr_case_locale.cpp
    ${ICU_ROOT}/icu4c/source/common/ucptrie.cpp
    ${ICU_ROOT}/icu4c/source/common/unorm.cpp
    ${ICU_ROOT}/icu4c/source/common/localeprioritylist.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnvbocu.cpp
    ${ICU_ROOT}/icu4c/source/common/servslkf.cpp
    ${ICU_ROOT}/icu4c/source/common/wintz.cpp
    ${ICU_ROOT}/icu4c/source/common/utrie_swap.cpp
    ${ICU_ROOT}/icu4c/source/common/bmpset.cpp
    ${ICU_ROOT}/icu4c/source/common/messagepattern.cpp
    ${ICU_ROOT}/icu4c/source/common/ustr_titlecase_brkiter.cpp
    ${ICU_ROOT}/icu4c/source/common/brkeng.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnv_ct.cpp
    ${ICU_ROOT}/icu4c/source/common/locdistance.cpp
    ${ICU_ROOT}/icu4c/source/common/ucurr.cpp
    ${ICU_ROOT}/icu4c/source/common/restrace.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnv_ext.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnvsel.cpp
    ${ICU_ROOT}/icu4c/source/common/ushape.cpp
    ${ICU_ROOT}/icu4c/source/common/umapfile.cpp
    ${ICU_ROOT}/icu4c/source/common/servlkf.cpp
    ${ICU_ROOT}/icu4c/source/common/ucln_cmn.cpp
    ${ICU_ROOT}/icu4c/source/common/ustrcase.cpp
    ${ICU_ROOT}/icu4c/source/common/uvectr32.cpp
    ${ICU_ROOT}/icu4c/source/common/rbbitblb.cpp
    ${ICU_ROOT}/icu4c/source/common/rbbi_cache.cpp
    ${ICU_ROOT}/icu4c/source/common/filterednormalizer2.cpp
    ${ICU_ROOT}/icu4c/source/common/ucmndata.cpp
    ${ICU_ROOT}/icu4c/source/common/charstr.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnv_set.cpp
    ${ICU_ROOT}/icu4c/source/common/rbbisetb.cpp
    ${ICU_ROOT}/icu4c/source/common/uniset_closure.cpp
    ${ICU_ROOT}/icu4c/source/common/putil.cpp
    ${ICU_ROOT}/icu4c/source/common/cstr.cpp
    ${ICU_ROOT}/icu4c/source/common/pluralmap.cpp
    ${ICU_ROOT}/icu4c/source/common/ucasemap.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnvmbcs.cpp
    ${ICU_ROOT}/icu4c/source/common/uniset.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnv.cpp
    ${ICU_ROOT}/icu4c/source/common/locbased.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnv_io.cpp
    ${ICU_ROOT}/icu4c/source/common/uchriter.cpp
    ${ICU_ROOT}/icu4c/source/common/ruleiter.cpp
    ${ICU_ROOT}/icu4c/source/common/ustrfmt.cpp
    ${ICU_ROOT}/icu4c/source/common/bytestriebuilder.cpp
    ${ICU_ROOT}/icu4c/source/common/cstring.cpp
    ${ICU_ROOT}/icu4c/source/common/servnotf.cpp
    ${ICU_ROOT}/icu4c/source/common/uset_props.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnv_u7.cpp
    ${ICU_ROOT}/icu4c/source/common/uidna.cpp
    ${ICU_ROOT}/icu4c/source/common/ubidi_props.cpp
    ${ICU_ROOT}/icu4c/source/common/servrbf.cpp
    ${ICU_ROOT}/icu4c/source/common/ucase.cpp
    ${ICU_ROOT}/icu4c/source/common/ures_cnv.cpp
    ${ICU_ROOT}/icu4c/source/common/unistr_props.cpp
    ${ICU_ROOT}/icu4c/source/common/static_unicode_sets.cpp
    ${ICU_ROOT}/icu4c/source/common/ucnv_u16.cpp
    ${ICU_ROOT}/icu4c/source/common/servlk.cpp
    ${ICU_ROOT}/icu4c/source/common/ustr_wcs.cpp
    ${ICU_ROOT}/icu4c/source/common/uvectr64.cpp
    ${ICU_ROOT}/icu4c/source/common/umath.cpp
    ${ICU_ROOT}/icu4c/source/common/stringpiece.cpp
    ${ICU_ROOT}/icu4c/source/common/ubidiwrt.cpp
    ${ICU_ROOT}/icu4c/source/common/cmemory.cpp
    ${ICU_ROOT}/icu4c/source/common/udataswp.cpp
    ${ICU_ROOT}/icu4c/source/common/cwchar.cpp
    ${ICU_ROOT}/icu4c/source/common/ucasemap_titlecase_brkiter.cpp
    ${ICU_ROOT}/icu4c/source/common/uprops.cpp
    ${ICU_ROOT}/icu4c/source/common/utrie2_builder.cpp
    ${ICU_ROOT}/icu4c/source/common/unistr_titlecase_brkiter.cpp
    ${ICU_ROOT}/icu4c/source/common/localebuilder.cpp
    ${ICU_ROOT}/icu4c/source/common/uloc_keytype.cpp
    ${ICU_ROOT}/icu4c/source/common/ubidi.cpp
    ${ICU_ROOT}/icu4c/source/common/locid.cpp
    ${ICU_ROOT}/icu4c/source/common/ucol_swp.cpp
    ${ICU_ROOT}/icu4c/source/common/rbbidata.cpp
    ${ICU_ROOT}/icu4c/source/common/locmap.cpp
    ${ICU_ROOT}/icu4c/source/common/brkiter.cpp
    ${ICU_ROOT}/icu4c/source/common/ustrtrns.cpp
    ${ICU_ROOT}/icu4c/source/common/udata.cpp
    ${ICU_ROOT}/icu4c/source/common/caniter.cpp
    ${ICU_ROOT}/icu4c/source/common/rbbistbl.cpp
    ${ICU_ROOT}/icu4c/source/common/locdispnames.cpp
    ${ICU_ROOT}/icu4c/source/common/utrie2.cpp
    ${ICU_ROOT}/icu4c/source/common/utf_impl.cpp
    ${ICU_ROOT}/icu4c/source/common/bytestrie.cpp
    ${ICU_ROOT}/icu4c/source/common/unifilt.cpp
    ${ICU_ROOT}/icu4c/source/common/emojiprops.cpp
    ${ICU_ROOT}/icu4c/source/common/lstmbe.cpp
    ${ICU_ROOT}/icu4c/source/ohos/init_data.cpp
)

add_library(hmicuuc.z STATIC ${ICUUC_SOURCES})

target_include_directories(hmicuuc.z
    SYSTEM PUBLIC ${ICU_ROOT}/icu4c/source/common
    PRIVATE ${ICU_ROOT}/icu4c/source/i18n
    PRIVATE ${ICU_ROOT}/icu4c/source
)

target_compile_definitions(hmicuuc.z PRIVATE
    -DU_ATTRIBUTE_DEPRECATED=
    -DU_COMMON_IMPLEMENTATION
    -DU_STATIC_IMPLEMENTATION
    -DUPRV_BLOCK_MACRO_BEGIN=
    -DUPRV_BLOCK_MACRO_END=
    -DUCONFIG_USE_WINDOWS_LCID_MAPPING_API=0
    -D_REENTRANT
    -DPIC
)

target_compile_options(hmicuuc.z PRIVATE
    -std=c++11 -frtti -fexceptions -Wno-shadow -Wno-unused
)

target_link_libraries(hmicuuc.z stubdata)

# --- hmicui18n.z (i18n) ---
set(ICUI18N_SOURCES
    ${ICU_ROOT}/icu4c/source/i18n/astro.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationtailoring.cpp
    ${ICU_ROOT}/icu4c/source/i18n/smpdtfmt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/calendar.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_utils.cpp
    ${ICU_ROOT}/icu4c/source/i18n/rbt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/dayperiodrules.cpp
    ${ICU_ROOT}/icu4c/source/i18n/utmscale.cpp
    ${ICU_ROOT}/icu4c/source/i18n/ulocdata.cpp
    ${ICU_ROOT}/icu4c/source/i18n/pluralranges.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_fluent.cpp
    ${ICU_ROOT}/icu4c/source/i18n/funcrepl.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationdatareader.cpp
    ${ICU_ROOT}/icu4c/source/i18n/quantityformatter.cpp
    ${ICU_ROOT}/icu4c/source/i18n/rematch.cpp
    ${ICU_ROOT}/icu4c/source/i18n/rbnf.cpp
    ${ICU_ROOT}/icu4c/source/i18n/uitercollationiterator.cpp
    ${ICU_ROOT}/icu4c/source/i18n/measure.cpp
    ${ICU_ROOT}/icu4c/source/i18n/uspoof_conf.cpp
    ${ICU_ROOT}/icu4c/source/i18n/units_complexconverter.cpp
    ${ICU_ROOT}/icu4c/source/i18n/casetrn.cpp
    ${ICU_ROOT}/icu4c/source/i18n/units_data.cpp
    ${ICU_ROOT}/icu4c/source/i18n/japancal.cpp
    ${ICU_ROOT}/icu4c/source/i18n/nfrs.cpp
    ${ICU_ROOT}/icu4c/source/i18n/double-conversion-cached-powers.cpp
    ${ICU_ROOT}/icu4c/source/i18n/dcfmtsym.cpp
    ${ICU_ROOT}/icu4c/source/i18n/olsontz.cpp
    ${ICU_ROOT}/icu4c/source/i18n/transreg.cpp
    ${ICU_ROOT}/icu4c/source/i18n/csdetect.cpp
    ${ICU_ROOT}/icu4c/source/i18n/coll.cpp
    ${ICU_ROOT}/icu4c/source/i18n/csrmbcs.cpp
    ${ICU_ROOT}/icu4c/source/i18n/numrange_impl.cpp
    ${ICU_ROOT}/icu4c/source/i18n/wintzimpl.cpp
    ${ICU_ROOT}/icu4c/source/i18n/currpinf.cpp
    ${ICU_ROOT}/icu4c/source/i18n/quant.cpp
    ${ICU_ROOT}/icu4c/source/i18n/tmunit.cpp
    ${ICU_ROOT}/icu4c/source/i18n/ucsdet.cpp
    ${ICU_ROOT}/icu4c/source/i18n/udateintervalformat.cpp
    ${ICU_ROOT}/icu4c/source/i18n/zrule.cpp
    ${ICU_ROOT}/icu4c/source/i18n/numfmt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/currunit.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_skeletons.cpp
    ${ICU_ROOT}/icu4c/source/i18n/nfrule.cpp
    ${ICU_ROOT}/icu4c/source/i18n/titletrn.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationdatabuilder.cpp
    ${ICU_ROOT}/icu4c/source/i18n/choicfmt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/rbt_set.cpp
    ${ICU_ROOT}/icu4c/source/i18n/fpositer.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_padding.cpp
    ${ICU_ROOT}/icu4c/source/i18n/csrutf8.cpp
    ${ICU_ROOT}/icu4c/source/i18n/numparse_currency.cpp
    ${ICU_ROOT}/icu4c/source/i18n/region.cpp
    ${ICU_ROOT}/icu4c/source/i18n/numparse_parsednumber.cpp
    ${ICU_ROOT}/icu4c/source/i18n/name2uni.cpp
    ${ICU_ROOT}/icu4c/source/i18n/repattrn.cpp
    ${ICU_ROOT}/icu4c/source/i18n/csrucode.cpp
    ${ICU_ROOT}/icu4c/source/i18n/sortkey.cpp
    ${ICU_ROOT}/icu4c/source/i18n/vzone.cpp
    ${ICU_ROOT}/icu4c/source/i18n/alphaindex.cpp
    ${ICU_ROOT}/icu4c/source/i18n/indiancal.cpp
    ${ICU_ROOT}/icu4c/source/i18n/strmatch.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationruleparser.cpp
    ${ICU_ROOT}/icu4c/source/i18n/nortrans.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_currencysymbols.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_capi.cpp
    ${ICU_ROOT}/icu4c/source/i18n/unumsys.cpp
    ${ICU_ROOT}/icu4c/source/i18n/uni2name.cpp
    ${ICU_ROOT}/icu4c/source/i18n/selfmt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/csrecog.cpp
    ${ICU_ROOT}/icu4c/source/i18n/ucln_in.cpp
    ${ICU_ROOT}/icu4c/source/i18n/coptccal.cpp
    ${ICU_ROOT}/icu4c/source/i18n/numparse_scientific.cpp
    ${ICU_ROOT}/icu4c/source/i18n/anytrans.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationcompare.cpp
    ${ICU_ROOT}/icu4c/source/i18n/nultrans.cpp
    ${ICU_ROOT}/icu4c/source/i18n/timezone.cpp
    ${ICU_ROOT}/icu4c/source/i18n/islamcal.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationiterator.cpp
    ${ICU_ROOT}/icu4c/source/i18n/toupptrn.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_grouping.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationrootelements.cpp
    ${ICU_ROOT}/icu4c/source/i18n/units_converter.cpp
    ${ICU_ROOT}/icu4c/source/i18n/numparse_validators.cpp
    ${ICU_ROOT}/icu4c/source/i18n/simpletz.cpp
    ${ICU_ROOT}/icu4c/source/i18n/numparse_impl.cpp
    ${ICU_ROOT}/icu4c/source/i18n/ufieldpositer.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_decimfmtprops.cpp
    ${ICU_ROOT}/icu4c/source/i18n/tridpars.cpp
    ${ICU_ROOT}/icu4c/source/i18n/numrange_fluent.cpp
    ${ICU_ROOT}/icu4c/source/i18n/translit.cpp
    ${ICU_ROOT}/icu4c/source/i18n/brktrans.cpp
    ${ICU_ROOT}/icu4c/source/i18n/tzgnames.cpp
    ${ICU_ROOT}/icu4c/source/i18n/unum.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationfcd.cpp
    ${ICU_ROOT}/icu4c/source/i18n/uregexc.cpp
    ${ICU_ROOT}/icu4c/source/i18n/basictz.cpp
    ${ICU_ROOT}/icu4c/source/i18n/double-conversion-fast-dtoa.cpp
    ${ICU_ROOT}/icu4c/source/i18n/bocsu.cpp
    ${ICU_ROOT}/icu4c/source/i18n/taiwncal.cpp
    ${ICU_ROOT}/icu4c/source/i18n/gender.cpp
    ${ICU_ROOT}/icu4c/source/i18n/units_router.cpp
    ${ICU_ROOT}/icu4c/source/i18n/reldtfmt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/uspoof.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_patternmodifier.cpp
    ${ICU_ROOT}/icu4c/source/i18n/utf16collationiterator.cpp
    ${ICU_ROOT}/icu4c/source/i18n/datefmt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/double-conversion-double-to-string.cpp
    ${ICU_ROOT}/icu4c/source/i18n/uspoof_build.cpp
    ${ICU_ROOT}/icu4c/source/i18n/remtrans.cpp
    ${ICU_ROOT}/icu4c/source/i18n/rbt_pars.cpp
    ${ICU_ROOT}/icu4c/source/i18n/search.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_output.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_scientific.cpp
    ${ICU_ROOT}/icu4c/source/i18n/numparse_affixes.cpp
    ${ICU_ROOT}/icu4c/source/i18n/currfmt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/hebrwcal.cpp
    ${ICU_ROOT}/icu4c/source/i18n/strrepl.cpp
    ${ICU_ROOT}/icu4c/source/i18n/listformatter.cpp
    ${ICU_ROOT}/icu4c/source/i18n/formatted_string_builder.cpp
    ${ICU_ROOT}/icu4c/source/i18n/cecal.cpp
    ${ICU_ROOT}/icu4c/source/i18n/numparse_decimal.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_decimalquantity.cpp
    ${ICU_ROOT}/icu4c/source/i18n/csr2022.cpp
    ${ICU_ROOT}/icu4c/source/i18n/plurrule.cpp
    ${ICU_ROOT}/icu4c/source/i18n/ucoleitr.cpp
    ${ICU_ROOT}/icu4c/source/i18n/uspoof_impl.cpp
    ${ICU_ROOT}/icu4c/source/i18n/rbt_rule.cpp
    ${ICU_ROOT}/icu4c/source/i18n/tzrule.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_modifiers.cpp
    ${ICU_ROOT}/icu4c/source/i18n/persncal.cpp
    ${ICU_ROOT}/icu4c/source/i18n/ucol_sit.cpp
    ${ICU_ROOT}/icu4c/source/i18n/decimfmt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/curramt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/winnmfmt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/scientificnumberformatter.cpp
    ${ICU_ROOT}/icu4c/source/i18n/udat.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_multiplier.cpp
    ${ICU_ROOT}/icu4c/source/i18n/string_segment.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_patternstring.cpp
    ${ICU_ROOT}/icu4c/source/i18n/tmutamt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/numrange_capi.cpp
    ${ICU_ROOT}/icu4c/source/i18n/gregoimp.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationroot.cpp
    ${ICU_ROOT}/icu4c/source/i18n/utf8collationiterator.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationdatawriter.cpp
    ${ICU_ROOT}/icu4c/source/i18n/regexst.cpp
    ${ICU_ROOT}/icu4c/source/i18n/msgfmt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_mapper.cpp
    ${ICU_ROOT}/icu4c/source/i18n/scriptset.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationfastlatinbuilder.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_notation.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationbuilder.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_asformat.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_affixutils.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationweights.cpp
    ${ICU_ROOT}/icu4c/source/i18n/tolowtrn.cpp
    ${ICU_ROOT}/icu4c/source/i18n/vtzone.cpp
    ${ICU_ROOT}/icu4c/source/i18n/tznames.cpp
    ${ICU_ROOT}/icu4c/source/i18n/double-conversion-bignum.cpp
    ${ICU_ROOT}/icu4c/source/i18n/formattedval_sbimpl.cpp
    ${ICU_ROOT}/icu4c/source/i18n/plurfmt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/dtitvfmt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/sharedbreakiterator.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationsets.cpp
    ${ICU_ROOT}/icu4c/source/i18n/double-conversion-strtod.cpp
    ${ICU_ROOT}/icu4c/source/i18n/ucol.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_symbolswrapper.cpp
    ${ICU_ROOT}/icu4c/source/i18n/rulebasedcollator.cpp
    ${ICU_ROOT}/icu4c/source/i18n/double-conversion-bignum-dtoa.cpp
    ${ICU_ROOT}/icu4c/source/i18n/decContext.cpp
    ${ICU_ROOT}/icu4c/source/i18n/uregex.cpp
    ${ICU_ROOT}/icu4c/source/i18n/compactdecimalformat.cpp
    ${ICU_ROOT}/icu4c/source/i18n/fmtable_cnv.cpp
    ${ICU_ROOT}/icu4c/source/i18n/reldatefmt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/windtfmt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/unesctrn.cpp
    ${ICU_ROOT}/icu4c/source/i18n/coleitr.cpp
    ${ICU_ROOT}/icu4c/source/i18n/dangical.cpp
    ${ICU_ROOT}/icu4c/source/i18n/dtitvinf.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_formatimpl.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collation.cpp
    ${ICU_ROOT}/icu4c/source/i18n/decNumber.cpp
    ${ICU_ROOT}/icu4c/source/i18n/regexcmp.cpp
    ${ICU_ROOT}/icu4c/source/i18n/format.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_usageprefs.cpp
    ${ICU_ROOT}/icu4c/source/i18n/dtrule.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_longnames.cpp
    ${ICU_ROOT}/icu4c/source/i18n/tmutfmt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/dtfmtsym.cpp
    ${ICU_ROOT}/icu4c/source/i18n/measunit_extra.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationfastlatin.cpp
    ${ICU_ROOT}/icu4c/source/i18n/umsg.cpp
    ${ICU_ROOT}/icu4c/source/i18n/tznames_impl.cpp
    ${ICU_ROOT}/icu4c/source/i18n/numsys.cpp
    ${ICU_ROOT}/icu4c/source/i18n/upluralrules.cpp
    ${ICU_ROOT}/icu4c/source/i18n/uregion.cpp
    ${ICU_ROOT}/icu4c/source/i18n/ucol_res.cpp
    ${ICU_ROOT}/icu4c/source/i18n/rbtz.cpp
    ${ICU_ROOT}/icu4c/source/i18n/erarules.cpp
    ${ICU_ROOT}/icu4c/source/i18n/regeximp.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationdata.cpp
    ${ICU_ROOT}/icu4c/source/i18n/utrans.cpp
    ${ICU_ROOT}/icu4c/source/i18n/ulistformatter.cpp
    ${ICU_ROOT}/icu4c/source/i18n/esctrn.cpp
    ${ICU_ROOT}/icu4c/source/i18n/zonemeta.cpp
    ${ICU_ROOT}/icu4c/source/i18n/tzfmt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/chnsecal.cpp
    ${ICU_ROOT}/icu4c/source/i18n/inputext.cpp
    ${ICU_ROOT}/icu4c/source/i18n/csrsbcs.cpp
    ${ICU_ROOT}/icu4c/source/i18n/csmatch.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationsettings.cpp
    ${ICU_ROOT}/icu4c/source/i18n/formattedval_iterimpl.cpp
    ${ICU_ROOT}/icu4c/source/i18n/collationkeys.cpp
    ${ICU_ROOT}/icu4c/source/i18n/formattedvalue.cpp
    ${ICU_ROOT}/icu4c/source/i18n/gregocal.cpp
    ${ICU_ROOT}/icu4c/source/i18n/dtptngen.cpp
    ${ICU_ROOT}/icu4c/source/i18n/ethpccal.cpp
    ${ICU_ROOT}/icu4c/source/i18n/buddhcal.cpp
    ${ICU_ROOT}/icu4c/source/i18n/fmtable.cpp
    ${ICU_ROOT}/icu4c/source/i18n/measfmt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/standardplural.cpp
    ${ICU_ROOT}/icu4c/source/i18n/rbt_data.cpp
    ${ICU_ROOT}/icu4c/source/i18n/tztrans.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_rounding.cpp
    ${ICU_ROOT}/icu4c/source/i18n/regextxt.cpp
    ${ICU_ROOT}/icu4c/source/i18n/smpdtfst.cpp
    ${ICU_ROOT}/icu4c/source/i18n/usearch.cpp
    ${ICU_ROOT}/icu4c/source/i18n/double-conversion-string-to-double.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_compact.cpp
    ${ICU_ROOT}/icu4c/source/i18n/cpdtrans.cpp
    ${ICU_ROOT}/icu4c/source/i18n/numparse_compositions.cpp
    ${ICU_ROOT}/icu4c/source/i18n/ucal.cpp
    ${ICU_ROOT}/icu4c/source/i18n/nfsubs.cpp
    ${ICU_ROOT}/icu4c/source/i18n/stsearch.cpp
    ${ICU_ROOT}/icu4c/source/i18n/ztrans.cpp
    ${ICU_ROOT}/icu4c/source/i18n/number_integerwidth.cpp
    ${ICU_ROOT}/icu4c/source/i18n/udatpg.cpp
    ${ICU_ROOT}/icu4c/source/i18n/measunit.cpp
    ${ICU_ROOT}/icu4c/source/i18n/fphdlimp.cpp
    ${ICU_ROOT}/icu4c/source/i18n/numparse_symbols.cpp
    ${ICU_ROOT}/icu4c/source/i18n/displayoptions.cpp
)

add_library(hmicui18n.z STATIC ${ICUI18N_SOURCES})

target_include_directories(hmicui18n.z
    SYSTEM PUBLIC ${ICU_ROOT}/icu4c/source/common
    PRIVATE ${ICU_ROOT}/icu4c/source/i18n
    PRIVATE ${ICU_ROOT}/icu4c/source
)

target_compile_definitions(hmicui18n.z PRIVATE
    -DU_ATTRIBUTE_DEPRECATED=
    -DU_COMMON_IMPLEMENTATION
    -DU_STATIC_IMPLEMENTATION
    -DU_I18N_IMPLEMENTATION
    -DUPRV_BLOCK_MACRO_BEGIN=
    -DUPRV_BLOCK_MACRO_END=
    -DUCONFIG_USE_WINDOWS_LCID_MAPPING_API=0
    -D_REENTRANT
    -DPIC
)

target_compile_options(hmicui18n.z PRIVATE
    -std=c++11 -frtti -fexceptions -Wno-shadow -Wno-unused
)

target_link_libraries(hmicui18n.z hmicuuc.z)

# ==============================================================================
# 3b. PCRE2 STATIC libraries — pcre2-8, pcre2-16
# Sources extracted from static_core/cmake/third_party/pcre2/CMakeLists.txt
# ==============================================================================
set(PCRE2_DIR ${THIRD_PARTY}/pcre2/pcre2)
set(PCRE2_SRC_DIR ${PCRE2_DIR}/src)
set(PCRE2_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/pcre2)
file(MAKE_DIRECTORY ${PCRE2_BIN_DIR})

set(PCRE2_SOURCES
    ${PCRE2_SRC_DIR}/pcre2_auto_possess.c
    ${PCRE2_SRC_DIR}/pcre2_chartables.c
    ${PCRE2_SRC_DIR}/pcre2_chkdint.c
    ${PCRE2_SRC_DIR}/pcre2_compile.c
    ${PCRE2_SRC_DIR}/pcre2_config.c
    ${PCRE2_SRC_DIR}/pcre2_context.c
    ${PCRE2_SRC_DIR}/pcre2_convert.c
    ${PCRE2_SRC_DIR}/pcre2_dfa_match.c
    ${PCRE2_SRC_DIR}/pcre2_error.c
    ${PCRE2_SRC_DIR}/pcre2_extuni.c
    ${PCRE2_SRC_DIR}/pcre2_find_bracket.c
    ${PCRE2_SRC_DIR}/pcre2_jit_compile.c
    ${PCRE2_SRC_DIR}/pcre2_maketables.c
    ${PCRE2_SRC_DIR}/pcre2_match.c
    ${PCRE2_SRC_DIR}/pcre2_match_data.c
    ${PCRE2_SRC_DIR}/pcre2_newline.c
    ${PCRE2_SRC_DIR}/pcre2_ord2utf.c
    ${PCRE2_SRC_DIR}/pcre2_pattern_info.c
    ${PCRE2_SRC_DIR}/pcre2_script_run.c
    ${PCRE2_SRC_DIR}/pcre2_serialize.c
    ${PCRE2_SRC_DIR}/pcre2_string_utils.c
    ${PCRE2_SRC_DIR}/pcre2_study.c
    ${PCRE2_SRC_DIR}/pcre2_substitute.c
    ${PCRE2_SRC_DIR}/pcre2_substring.c
    ${PCRE2_SRC_DIR}/pcre2_tables.c
    ${PCRE2_SRC_DIR}/pcre2_ucd.c
    ${PCRE2_SRC_DIR}/pcre2_valid_utf.c
    ${PCRE2_SRC_DIR}/pcre2_xclass.c
)

# Generate PCRE2 config.h for Android
# Variables for #cmakedefine expansion in config-cmake.h.in
set(HAVE_BUILTIN_MUL_OVERFLOW 1)
set(HAVE_ATTRIBUTE_UNINITIALIZED 1)
set(HAVE_DIRENT_H 1)
set(HAVE_SYS_STAT_H 1)
set(HAVE_SYS_TYPES_H 1)
set(HAVE_UNISTD_H 1)
set(HAVE_MEMMOVE 1)
set(HAVE_STRERROR 1)
set(SUPPORT_PCRE2_8 1)
set(SUPPORT_PCRE2_16 1)
set(SUPPORT_UNICODE 1)
set(PCRE2_LINK_SIZE 2)
set(PCRE2_PARENS_NEST_LIMIT 250)
set(PCRE2_HEAP_LIMIT 20000000)
set(PCRE2_MATCH_LIMIT 10000000)
set(PCRE2_MATCH_LIMIT_DEPTH MATCH_LIMIT)
set(PCRE2GREP_BUFSIZE 20480)
set(PCRE2GREP_MAX_BUFSIZE 1048576)
set(NEWLINE_DEFAULT 2)
set(PCRE2_MAJOR 10)
set(PCRE2_MINOR 44)
set(PCRE2_PRERELEASE "")
set(PCRE2_DATE "2024-06-07")
set(PCRE2_EXPORT "")
set(PCRE2_MAX_VARLOOKBEHIND 255)

configure_file(${PCRE2_DIR}/config-cmake.h.in ${PCRE2_BIN_DIR}/config.h @ONLY)
configure_file(${PCRE2_SRC_DIR}/pcre2.h.in ${PCRE2_BIN_DIR}/pcre2.h @ONLY)

# --- pcre2-8 (8-bit code unit) ---
add_library(pcre2-8 STATIC ${PCRE2_SOURCES} ${PCRE2_BIN_DIR}/config.h)

target_include_directories(pcre2-8
    SYSTEM PUBLIC ${PCRE2_SRC_DIR}
    PUBLIC ${PCRE2_BIN_DIR}
)

target_compile_definitions(pcre2-8 PRIVATE
    PCRE2_CODE_UNIT_WIDTH=8
    PCRE2_STATIC
    HAVE_CONFIG_H
    SUPPORT_UNICODE=1
    SUPPORT_JIT=0
)

set_target_properties(pcre2-8 PROPERTIES
    OUTPUT_NAME pcre2-8
    C_STANDARD 11
)

# --- pcre2-16 (16-bit code unit) ---
add_library(pcre2-16 STATIC ${PCRE2_SOURCES} ${PCRE2_BIN_DIR}/config.h)

target_include_directories(pcre2-16
    SYSTEM PUBLIC ${PCRE2_SRC_DIR}
    PUBLIC ${PCRE2_BIN_DIR}
)

target_compile_definitions(pcre2-16 PRIVATE
    PCRE2_CODE_UNIT_WIDTH=16
    PCRE2_STATIC
    HAVE_CONFIG_H
    SUPPORT_UNICODE=1
    SUPPORT_JIT=0
)

set_target_properties(pcre2-16 PROPERTIES
    OUTPUT_NAME pcre2-16
    C_STANDARD 11
)

# ==============================================================================
# 4. libpandabase (arkbase) - base library
# ==============================================================================
set(LIBPANDABASE_SOURCES
    # Core
    ${SRC_ROOT}/libpandabase/mem/alloc_tracker.cpp
    ${SRC_ROOT}/libpandabase/mem/code_allocator.cpp
    ${SRC_ROOT}/libpandabase/mem/pool_manager.cpp
    ${SRC_ROOT}/libpandabase/mem/mem_config.cpp
    ${SRC_ROOT}/libpandabase/utils/debug.cpp
    ${SRC_ROOT}/libpandabase/utils/logger.cpp
    ${SRC_ROOT}/libpandabase/utils/dfx.cpp
    ${SRC_ROOT}/libpandabase/utils/terminate.cpp
    ${SRC_ROOT}/libpandabase/utils/utils.cpp
    ${SRC_ROOT}/libpandabase/trace/trace.cpp
    ${SRC_ROOT}/libpandabase/os/filesystem.cpp
    ${SRC_ROOT}/libpandabase/os/native_stack.cpp
    ${SRC_ROOT}/libpandabase/os/property.cpp
    ${SRC_ROOT}/libpandabase/os/dfx_option.cpp
    ${SRC_ROOT}/libpandabase/taskmanager/task.cpp
    ${SRC_ROOT}/libpandabase/taskmanager/task_manager.cpp
    ${SRC_ROOT}/libpandabase/taskmanager/task_queue_set.cpp
    ${SRC_ROOT}/libpandabase/taskmanager/task_scheduler.cpp
    ${SRC_ROOT}/libpandabase/taskmanager/worker_thread.cpp
    ${SRC_ROOT}/libpandabase/taskmanager/utils/task_time_stats.cpp
    # LTO sources
    ${SRC_ROOT}/libpandabase/mem/arena_allocator.cpp
    ${SRC_ROOT}/libpandabase/mem/base_mem_stats.cpp
    ${SRC_ROOT}/libpandabase/mem/pool_map.cpp
    ${SRC_ROOT}/libpandabase/os/time.cpp
    ${SRC_ROOT}/libpandabase/utils/json_builder.cpp
    ${SRC_ROOT}/libpandabase/utils/json_parser.cpp
    ${SRC_ROOT}/libpandabase/utils/time.cpp
    ${SRC_ROOT}/libpandabase/utils/type_converter.cpp
    ${SRC_ROOT}/libpandabase/utils/utf.cpp
    # Arch: aarch64
    ${SRC_ROOT}/libpandabase/arch/aarch64/cpu_features.cpp
    # Unix platform
    ${SRC_ROOT}/platforms/unix/libpandabase/cpu_affinity.cpp
    ${SRC_ROOT}/platforms/unix/libpandabase/error.cpp
    ${SRC_ROOT}/platforms/unix/libpandabase/native_stack.cpp
    ${SRC_ROOT}/platforms/unix/libpandabase/property.cpp
    ${SRC_ROOT}/platforms/unix/libpandabase/file.cpp
    ${SRC_ROOT}/platforms/unix/libpandabase/filesystem.cpp
    ${SRC_ROOT}/platforms/unix/libpandabase/kill.cpp
    ${SRC_ROOT}/platforms/unix/libpandabase/library_loader_resolve_symbol.cpp
    ${SRC_ROOT}/platforms/unix/libpandabase/mem.cpp
    ${SRC_ROOT}/platforms/unix/libpandabase/mem_hooks.cpp
    ${SRC_ROOT}/platforms/unix/libpandabase/sighook.cpp
    ${SRC_ROOT}/platforms/unix/libpandabase/system_environment.cpp
    ${SRC_ROOT}/platforms/unix/libpandabase/trace.cpp
    ${SRC_ROOT}/platforms/unix/libpandabase/exec.cpp
    ${SRC_ROOT}/platforms/unix/libpandabase/pipe.cpp
    # Mobile overrides
    ${SRC_ROOT}/platforms/mobile/libpandabase/library_loader_load.cpp
    # Futex (PANDA_USE_FUTEX)
    ${SRC_ROOT}/platforms/unix/libpandabase/futex/fmutex.cpp
    ${SRC_ROOT}/platforms/unix/libpandabase/futex/mutex.cpp
    # Stacktrace (stub for product/mobile builds - no libdwarf)
    ${SRC_ROOT}/libpandabase/os/stacktrace_stub.cpp
    # LTO unix thread
    ${SRC_ROOT}/platforms/unix/libpandabase/thread.cpp
)

add_library(pandabase SHARED ${LIBPANDABASE_SOURCES})

target_include_directories(pandabase
    PUBLIC ${SRC_ROOT}
    PUBLIC ${SRC_ROOT}/libpandabase
    PUBLIC ${GEN_ROOT}/generated
    PUBLIC ${GEN_ROOT}/panda_gen_options/generated
    PUBLIC ${GEN_ROOT}/libpandabase/generated
    PUBLIC ${THIRD_PARTY}/utils_native/base/include
    PRIVATE ${SRC_ROOT}/platforms/mobile/libpandabase
)

target_compile_definitions(pandabase PRIVATE PANDA_BUILD_PANDABASE=1)

target_link_libraries(pandabase
    c_secshared
    dl
    atomic
)

# ==============================================================================
# 5. libziparchive (arkziparchive) - zip archive support
# ==============================================================================
set(LIBZIPARCHIVE_SOURCES
    ${SRC_ROOT}/libziparchive/zip_archive.cpp
    ${SRC_ROOT}/libziparchive/extractortool/extractor.cpp
    ${SRC_ROOT}/libziparchive/extractortool/file_mapper.cpp
    ${SRC_ROOT}/libziparchive/extractortool/zip_file_reader_io.cpp
    ${SRC_ROOT}/libziparchive/extractortool/zip_file_reader.cpp
    ${SRC_ROOT}/libziparchive/extractortool/zip_file.cpp
)

add_library(ziparchive SHARED ${LIBZIPARCHIVE_SOURCES})

target_include_directories(ziparchive
    PUBLIC ${SRC_ROOT}
    PUBLIC ${SRC_ROOT}/libziparchive
    PUBLIC ${ZLIB_ROOT}
    PUBLIC ${ZLIB_ROOT}/contrib/minizip
    PUBLIC ${THIRD_PARTY}/utils_native/base/include
)

target_link_libraries(ziparchive
    pandabase
    z
    c_secshared
)

# ==============================================================================
# 6. libpandafile (arkfile) - ABC/PANDA file reader
# ==============================================================================
set(LIBPANDAFILE_SOURCES
    ${SRC_ROOT}/libpandafile/annotation_data_accessor.cpp
    ${SRC_ROOT}/libpandafile/bytecode_emitter.cpp
    ${SRC_ROOT}/libpandafile/debug_data_accessor.cpp
    ${SRC_ROOT}/libpandafile/debug_helpers.cpp
    ${SRC_ROOT}/libpandafile/field_data_accessor.cpp
    ${SRC_ROOT}/libpandafile/file.cpp
    ${SRC_ROOT}/libpandafile/file_reader.cpp
    ${SRC_ROOT}/libpandafile/file_writer.cpp
    ${SRC_ROOT}/libpandafile/file_items.cpp
    ${SRC_ROOT}/libpandafile/file_item_container.cpp
    ${SRC_ROOT}/libpandafile/pgo.cpp
    ${SRC_ROOT}/libpandafile/class_data_accessor.cpp
    ${SRC_ROOT}/libpandafile/code_data_accessor.cpp
    ${SRC_ROOT}/libpandafile/method_data_accessor.cpp
    ${SRC_ROOT}/libpandafile/method_handle_data_accessor.cpp
    ${SRC_ROOT}/libpandafile/debug_info_extractor.cpp
    ${SRC_ROOT}/libpandafile/literal_data_accessor.cpp
    ${SRC_ROOT}/libpandafile/file_format_version.cpp
)

add_library(pandafile SHARED ${LIBPANDAFILE_SOURCES})

target_include_directories(pandafile
    PUBLIC ${SRC_ROOT}
    PUBLIC ${SRC_ROOT}/libpandafile
    PUBLIC ${GEN_ROOT}/libpandafile/include
    PUBLIC ${GEN_ROOT}/libpandabase/generated
    PUBLIC ${GEN_ROOT}/generated
    PUBLIC ${THIRD_PARTY}/utils_native/base/include
)

target_compile_definitions(pandafile PRIVATE PANDA_BUILD_PANDAFILE=1)

target_compile_options(pandafile
    PRIVATE -Wno-invalid-offsetof
)

target_link_libraries(pandafile
    pandabase
    ziparchive
    c_secshared
    atomic
)

# ==============================================================================
# 7. libvixl (STATIC) - ARM64 code generation helper
# ==============================================================================
set(VIXL_ROOT ${THIRD_PARTY}/vixl)

set(VIXL_COMMON_SOURCES
    ${VIXL_ROOT}/src/code-buffer-vixl.cc
    ${VIXL_ROOT}/src/compiler-intrinsics-vixl.cc
    ${VIXL_ROOT}/src/cpu-features.cc
    ${VIXL_ROOT}/src/utils-vixl.cc
)

set(VIXL_AARCH64_SOURCES
    ${VIXL_ROOT}/src/aarch64/assembler-aarch64.cc
    ${VIXL_ROOT}/src/aarch64/operands-aarch64.cc
    ${VIXL_ROOT}/src/aarch64/cpu-aarch64.cc
    ${VIXL_ROOT}/src/aarch64/instructions-aarch64.cc
    ${VIXL_ROOT}/src/aarch64/macro-assembler-aarch64.cc
    ${VIXL_ROOT}/src/aarch64/decoder-aarch64.cc
    ${VIXL_ROOT}/src/aarch64/disasm-aarch64.cc
    ${VIXL_ROOT}/src/aarch64/logic-aarch64.cc
    ${VIXL_ROOT}/src/aarch64/pointer-auth-aarch64.cc
    ${VIXL_ROOT}/src/aarch64/cpu-features-auditor-aarch64.cc
    ${VIXL_ROOT}/src/aarch64/debugger-aarch64.cc
    ${VIXL_ROOT}/src/aarch64/simulator-aarch64.cc
)

add_library(vixl STATIC
    ${VIXL_COMMON_SOURCES}
    ${VIXL_AARCH64_SOURCES}
)

target_include_directories(vixl
    PUBLIC ${VIXL_ROOT}
    PUBLIC ${VIXL_ROOT}/src
    PUBLIC ${VIXL_ROOT}/src/aarch64
)

target_compile_definitions(vixl
    PRIVATE VIXL_CODE_BUFFER_MMAP
    PRIVATE PANDA_BUILD
    PRIVATE VIXL_INCLUDE_SIMULATOR_AARCH64
    PRIVATE VIXL_INCLUDE_TARGET_A64
    PRIVATE VIXL_USE_PANDA_ALLOC
)
target_compile_options(vixl PRIVATE -Wno-shadow -Wno-deprecated-declarations)
target_link_libraries(vixl pandabase)

# ==============================================================================
# 8. libdprof (STATIC) - profiling data + IPC
# ==============================================================================
set(DPROF_ROOT ${SRC_ROOT}/dprof/libdprof)

add_library(dprof STATIC
    ${DPROF_ROOT}/dprof/ipc/ipc_message.cpp
    ${DPROF_ROOT}/dprof/profiling_data.cpp
    ${DPROF_ROOT}/dprof/ipc/ipc_unix_socket.cpp
)

target_include_directories(dprof
    PUBLIC ${DPROF_ROOT}
)

target_link_libraries(dprof pandabase)

# ==============================================================================
# 9. libarktarget_options - target-specific defaults
# ==============================================================================
add_library(arktarget_options SHARED
    ${SRC_ROOT}/platforms/common/default_target_options.cpp
)

target_include_directories(arktarget_options
    PUBLIC ${SRC_ROOT}
)

# ==============================================================================
# 10. libarkassembler - Panda assembler + ETS extension (runtime TypeAPI)
# ==============================================================================
set(ASSEMBLER_ROOT ${SRC_ROOT}/assembler)

set(ASSEMBLER_SOURCES
    ${ASSEMBLER_ROOT}/lexer.cpp
    ${ASSEMBLER_ROOT}/annotation.cpp
    ${ASSEMBLER_ROOT}/assembly-emitter.cpp
    ${ASSEMBLER_ROOT}/assembly-parser.cpp
    ${ASSEMBLER_ROOT}/assembly-program.cpp
    ${ASSEMBLER_ROOT}/assembly-type.cpp
    ${ASSEMBLER_ROOT}/assembly-ins.cpp
    ${ASSEMBLER_ROOT}/context.cpp
    ${ASSEMBLER_ROOT}/meta.cpp
    ${ASSEMBLER_ROOT}/pandasm.cpp
    ${ASSEMBLER_ROOT}/extensions/extensions.cpp
    ${ASSEMBLER_ROOT}/utils/number-utils.cpp
    # Generated: ins_to_string.cpp (from isa.yaml template)
    ${GEN_ROOT}/assembler/ins_to_string.cpp
    # ETS extension (runtime TypeAPI bytecode generation)
    ${ETS_PLUGIN_ROOT}/assembler/extension/ets_meta.cpp
)

add_library(arkassembler SHARED ${ASSEMBLER_SOURCES})

target_include_directories(arkassembler
    PUBLIC ${SRC_ROOT}
    PUBLIC ${ASSEMBLER_ROOT}
    PUBLIC ${GEN_ROOT}
    PUBLIC ${GEN_ROOT}/assembler
    PUBLIC ${GEN_ROOT}/generated
    PUBLIC ${GEN_ROOT}/libpandafile/include
    PUBLIC ${GEN_ROOT}/libpandabase/generated
)

target_link_libraries(arkassembler
    pandabase
    pandafile
)

# ==============================================================================
# 11. libarkencoder (STATIC) - encoder/codegen support (aarch64)
# ==============================================================================
set(ENCODER_ROOT ${SRC_ROOT}/compiler/optimizer/code_generator)

add_library(arkencoder STATIC
    ${ENCODER_ROOT}/target/target.cpp
    ${ENCODER_ROOT}/target/asm_printer.cpp
    ${ENCODER_ROOT}/target/aarch64/encode.cpp
    ${ENCODER_ROOT}/target/aarch64/callconv.cpp
    ${ENCODER_ROOT}/target/aarch64/regfile.cpp
)

target_include_directories(arkencoder
    PUBLIC ${ENCODER_ROOT}
    PRIVATE ${SRC_ROOT}/compiler
)

target_compile_options(arkencoder PRIVATE -Wno-shadow -Wno-deprecated-declarations)
target_compile_definitions(arkencoder PRIVATE USE_VIXL_ARM64 VIXL_CODE_BUFFER_MMAP)

target_link_libraries(arkencoder pandabase vixl)

# ==============================================================================
# 12. libarkcompiler - compiler optimizer + code_info
# ==============================================================================
set(COMPILER_ROOT ${SRC_ROOT}/compiler)

set(OPTIMIZER_SOURCES
    ${COMPILER_ROOT}/optimizer/pass.cpp
    ${COMPILER_ROOT}/optimizer/pass_manager.cpp
    ${COMPILER_ROOT}/optimizer/pass_manager_statistics.cpp
    ${COMPILER_ROOT}/optimizer/pipeline.cpp
    ${COMPILER_ROOT}/optimizer/analysis/alias_analysis.cpp
    ${COMPILER_ROOT}/optimizer/analysis/alias_visitor.cpp
    ${COMPILER_ROOT}/optimizer/analysis/bounds_analysis.cpp
    ${COMPILER_ROOT}/optimizer/analysis/countable_loop_parser.cpp
    ${COMPILER_ROOT}/optimizer/analysis/catch_inputs.cpp
    ${COMPILER_ROOT}/optimizer/analysis/dominators_tree.cpp
    ${COMPILER_ROOT}/optimizer/analysis/linear_order.cpp
    ${COMPILER_ROOT}/optimizer/analysis/liveness_analyzer.cpp
    ${COMPILER_ROOT}/optimizer/analysis/liveness_use_table.cpp
    ${COMPILER_ROOT}/optimizer/analysis/live_in_analysis.cpp
    ${COMPILER_ROOT}/optimizer/analysis/live_registers.cpp
    ${COMPILER_ROOT}/optimizer/analysis/loop_analyzer.cpp
    ${COMPILER_ROOT}/optimizer/analysis/monitor_analysis.cpp
    ${COMPILER_ROOT}/optimizer/analysis/object_type_propagation.cpp
    ${COMPILER_ROOT}/optimizer/analysis/rpo.cpp
    ${COMPILER_ROOT}/optimizer/analysis/reg_alloc_verifier.cpp
    ${COMPILER_ROOT}/optimizer/analysis/types_analysis.cpp
    ${COMPILER_ROOT}/optimizer/ir/analysis.cpp
    ${COMPILER_ROOT}/optimizer/ir/basicblock.cpp
    ${COMPILER_ROOT}/optimizer/ir/dump.cpp
    ${COMPILER_ROOT}/optimizer/ir/graph.cpp
    ${COMPILER_ROOT}/optimizer/ir/inst.cpp
    ${COMPILER_ROOT}/optimizer/ir/locations.cpp
    ${COMPILER_ROOT}/optimizer/ir/visualizer_printer.cpp
    ${COMPILER_ROOT}/optimizer/ir/graph_checker.cpp
    ${COMPILER_ROOT}/optimizer/ir/graph_cloner.cpp
    ${COMPILER_ROOT}/optimizer/ir/loop_unswitcher.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/adjust_arefs.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/balance_expressions.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/branch_elimination.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/checks_elimination.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/code_sink.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/const_folding.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/deoptimize_elimination.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/escape.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/cleanup.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/if_conversion.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/if_merging.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/licm.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/licm_conditions.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/condition_chain_manager.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/condition_chain_cache.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/locations_builder.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/loop_idioms.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/loop_peeling.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/loop_unswitch.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/loop_unroll.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/lse.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/lower_boxed_boolean.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/memory_barriers.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/memory_coalescing.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/native_call_optimization.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/object_type_check_elimination.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/optimize_string_concat.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/peepholes.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/phi_type_resolving.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/redundant_loop_elimination.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/reserve_string_builder_buffer.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/savestate_optimization.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/scheduler.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/simplify_string_builder.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/string_builder_utils.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/try_catch_resolving.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/inline_intrinsics.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/vn.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/cse.cpp
    ${COMPILER_ROOT}/tools/debug/jit_writer.cpp
    ${COMPILER_ROOT}/compiler_logger.cpp
    ${COMPILER_ROOT}/compiler_options.cpp
)

set(COMPILER_SOURCES
    ${OPTIMIZER_SOURCES}
    # Additional optimizer sources
    ${COMPILER_ROOT}/optimizer/ir/aot_data.cpp
    ${COMPILER_ROOT}/optimizer/ir_builder/ir_builder.cpp
    ${COMPILER_ROOT}/optimizer/ir_builder/inst_builder.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/inlining.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/lowering.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/move_constants.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/regalloc/cleanup_empty_blocks.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/regalloc/reg_alloc_base.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/regalloc/interference_graph.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/regalloc/reg_alloc.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/regalloc/reg_alloc_stat.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/regalloc/reg_alloc_graph_coloring.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/regalloc/reg_map.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/regalloc/reg_alloc_linear_scan.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/regalloc/spill_fills_resolver.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/regalloc/split_resolver.cpp
    ${COMPILER_ROOT}/optimizer/optimizations/regalloc/reg_alloc_resolver.cpp
    ${COMPILER_ROOT}/optimizer/code_generator/encode.cpp
    # Codegen + code_info (non-macOS path)
    ${COMPILER_ROOT}/compile_method.cpp
    ${COMPILER_ROOT}/optimizer/code_generator/disassembly.cpp
    ${COMPILER_ROOT}/optimizer/code_generator/codegen.cpp
    ${COMPILER_ROOT}/optimizer/code_generator/codegen_native.cpp
    ${COMPILER_ROOT}/optimizer/code_generator/encode_visitor.cpp
    ${COMPILER_ROOT}/optimizer/code_generator/spill_fill_encoder.cpp
    ${COMPILER_ROOT}/optimizer/code_generator/slow_path.cpp
    ${COMPILER_ROOT}/optimizer/code_generator/method_properties.cpp
    ${COMPILER_ROOT}/code_info/code_info.cpp
    ${COMPILER_ROOT}/code_info/code_info_builder.cpp
    # Auto-generated inst builder
    ${GEN_ROOT}/compiler/inst_builder_gen.cpp
    # ETS plugin compiler sources
    ${ETS_PLUGIN_ROOT}/compiler/codegen_intrinsics_ets.cpp
    ${ETS_PLUGIN_ROOT}/compiler/ir_build_intrinsics_ets.cpp
    ${ETS_PLUGIN_ROOT}/compiler/optimizer/ir_builder/ets_inst_builder.cpp
    ${ETS_PLUGIN_ROOT}/compiler/optimizer/ets_intrinsics_inlining_expansion.cpp
    ${ETS_PLUGIN_ROOT}/compiler/optimizer/ets_intrinsics_peephole.cpp
    ${ETS_PLUGIN_ROOT}/compiler/optimizer/ets_codegen_extensions.cpp
)

add_library(arkcompiler SHARED ${COMPILER_SOURCES})

target_include_directories(arkcompiler
    PUBLIC ${SRC_ROOT}
    PUBLIC ${SRC_ROOT}/runtime
    PUBLIC ${SRC_ROOT}/compiler
    PUBLIC ${GEN_ROOT}/compiler
    PUBLIC ${GEN_ROOT}/generated
)

target_compile_options(arkcompiler PRIVATE -Wno-invalid-offsetof)
target_compile_options(arkcompiler PRIVATE -Wno-shadow -Wno-deprecated-declarations)

target_link_libraries(arkcompiler
    pandabase
    pandafile
    arkencoder
)

# ==============================================================================
# 13. libarkaotmanager - AOT file manager
# ==============================================================================
add_library(arkaotmanager SHARED
    ${COMPILER_ROOT}/aot/aot_file.cpp
    ${COMPILER_ROOT}/aot/aot_manager.cpp
)

target_include_directories(arkaotmanager
    PUBLIC ${SRC_ROOT}
    PUBLIC ${SRC_ROOT}/runtime
    PUBLIC ${GEN_ROOT}/compiler
)

target_link_libraries(arkaotmanager
    pandabase
    pandafile
    arkcompiler
)

# ==============================================================================
# 14. libhongengine_c - C API wrapper for JNI bridge
# ==============================================================================
add_library(hongengine_c SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/hongengine_c_api.cpp
)

target_include_directories(hongengine_c
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PUBLIC ${SRC_ROOT}
    PUBLIC ${SRC_ROOT}/libpandafile
    PUBLIC ${GEN_ROOT}/libpandafile/include
    PUBLIC ${GEN_ROOT}/libpandabase/generated
    PUBLIC ${GEN_ROOT}/generated
    PUBLIC ${SRC_ROOT}/plugins/ets/runtime
    PUBLIC ${SRC_ROOT}/plugins/ets
    PUBLIC ${GEN_ROOT}/panda_gen_options
    PUBLIC ${GEN_ROOT}/runtime/include
)

target_link_libraries(hongengine_c
    arkruntime
)

# ==============================================================================
# 14b. hongengine_test — standalone ARM64 executable for host testing via QEMU
# ==============================================================================
add_executable(hongengine_test
    ${CMAKE_CURRENT_SOURCE_DIR}/host_test_main.cpp
)

target_include_directories(hongengine_test
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${SRC_ROOT}
    PRIVATE ${SRC_ROOT}/libpandafile
    PRIVATE ${GEN_ROOT}/libpandafile/include
    PRIVATE ${GEN_ROOT}/libpandabase/generated
    PRIVATE ${GEN_ROOT}/generated
    PRIVATE ${SRC_ROOT}/plugins/ets/runtime
    PRIVATE ${SRC_ROOT}/plugins/ets
    PRIVATE ${GEN_ROOT}/panda_gen_options
    PRIVATE ${GEN_ROOT}/runtime/include
)

target_link_libraries(hongengine_test
    hongengine_c
    arkruntime
)

# ==============================================================================
# 15. libarkruntime - core VM runtime + ETS plugin
# ==============================================================================
set(RUNTIME_ROOT ${SRC_ROOT}/runtime)
set(VERIFICATION_ROOT ${SRC_ROOT}/verification)

set(RUNTIME_SOURCES
    # --- Main runtime sources (from runtime/CMakeLists.txt) ---
    ${RUNTIME_ROOT}/assert_gc_scope.cpp
    ${RUNTIME_ROOT}/bridge/bridge.cpp
    ${RUNTIME_ROOT}/entrypoints/entrypoints.cpp
    ${RUNTIME_ROOT}/exceptions.cpp
    ${RUNTIME_ROOT}/class_linker.cpp
    ${RUNTIME_ROOT}/class_linker_extension.cpp
    ${RUNTIME_ROOT}/class_initializer.cpp
    ${RUNTIME_ROOT}/tooling/backtrace/backtrace.cpp
    ${RUNTIME_ROOT}/tooling/debugger.cpp
    ${RUNTIME_ROOT}/tooling/evaluation/expression_loader.cpp
    ${RUNTIME_ROOT}/tooling/inspector/debugger_arkapi.cpp
    ${RUNTIME_ROOT}/tooling/pt_default_lang_extension.cpp
    ${RUNTIME_ROOT}/tooling/pt_thread.cpp
    ${RUNTIME_ROOT}/tooling/debug_inf.cpp
    ${RUNTIME_ROOT}/tooling/tools.cpp
    ${RUNTIME_ROOT}/tooling/sampler/sampling_profiler.cpp
    ${RUNTIME_ROOT}/tooling/sampler/sample_writer.cpp
    ${RUNTIME_ROOT}/tooling/sampler/samples_record.cpp
    ${RUNTIME_ROOT}/tooling/sampler/thread_communicator.cpp
    ${RUNTIME_ROOT}/tooling/sampler/stack_walker_base.cpp
    ${RUNTIME_ROOT}/tooling/sampler/lock_free_queue.cpp
    ${RUNTIME_ROOT}/field.cpp
    ${RUNTIME_ROOT}/gc_task.cpp
    ${RUNTIME_ROOT}/dprofiler/dprofiler.cpp
    ${RUNTIME_ROOT}/hotreload/hotreload.cpp
    ${RUNTIME_ROOT}/interpreter/interpreter.cpp
    ${RUNTIME_ROOT}/interpreter/runtime_interface.cpp
    ${RUNTIME_ROOT}/intrinsics.cpp
    ${RUNTIME_ROOT}/coretypes/string.cpp
    ${RUNTIME_ROOT}/coretypes/array.cpp
    ${RUNTIME_ROOT}/class.cpp
    ${RUNTIME_ROOT}/class_helper.cpp
    ${RUNTIME_ROOT}/locks.cpp
    ${RUNTIME_ROOT}/panda_vm.cpp
    ${RUNTIME_ROOT}/language_context.cpp
    ${RUNTIME_ROOT}/mem/gc/epsilon/epsilon.cpp
    ${RUNTIME_ROOT}/mem/gc/epsilon-g1/epsilon-g1.cpp
    ${RUNTIME_ROOT}/mem/gc/epsilon/epsilon_barrier.cpp
    ${RUNTIME_ROOT}/mem/gc/gc.cpp
    ${RUNTIME_ROOT}/mem/gc/gc_adaptive_marking_stack.cpp
    ${RUNTIME_ROOT}/mem/gc/gc_settings.cpp
    ${RUNTIME_ROOT}/mem/gc/lang/gc_lang.cpp
    ${RUNTIME_ROOT}/mem/gc/gc_queue.cpp
    ${RUNTIME_ROOT}/mem/gc/gc_root.cpp
    ${RUNTIME_ROOT}/mem/gc/gc_root_type.cpp
    ${RUNTIME_ROOT}/mem/gc/gc_stats.cpp
    ${RUNTIME_ROOT}/mem/gc/gc_trigger.cpp
    ${RUNTIME_ROOT}/mem/gc/gc-hung/gc_hung.cpp
    ${RUNTIME_ROOT}/mem/gc/card_table.cpp
    ${RUNTIME_ROOT}/mem/gc/heap-space-misc/crossing_map.cpp
    ${RUNTIME_ROOT}/mem/gc/heap-space-misc/crossing_map_singleton.cpp
    ${RUNTIME_ROOT}/mem/gc/bitmap.cpp
    ${RUNTIME_ROOT}/mem/gc/gc_scope.cpp
    ${RUNTIME_ROOT}/mem/gc/gc_scoped_phase.cpp
    ${RUNTIME_ROOT}/mem/gc/workers/gc_worker.cpp
    ${RUNTIME_ROOT}/mem/gc/workers/gc_workers_task_pool.cpp
    ${RUNTIME_ROOT}/mem/gc/workers/gc_workers_task_queue.cpp
    ${RUNTIME_ROOT}/mem/gc/workers/gc_workers_thread_pool.cpp
    ${RUNTIME_ROOT}/mem/gc/stw-gc/stw-gc.cpp
    ${RUNTIME_ROOT}/mem/gc/gc_barrier_set.cpp
    ${RUNTIME_ROOT}/mem/gc/gen-gc/gen-gc.cpp
    ${RUNTIME_ROOT}/mem/gc/cmc-gc-adapter/cmc-gc-adapter.cpp
    ${RUNTIME_ROOT}/mem/gc/cmc-gc-adapter/cmc-allocator-adapter.cpp
    ${RUNTIME_ROOT}/mem/refstorage/ref_block.cpp
    ${RUNTIME_ROOT}/mem/refstorage/reference_storage.cpp
    ${RUNTIME_ROOT}/mem/refstorage/global_object_storage.cpp
    ${RUNTIME_ROOT}/mem/allocator.cpp
    ${RUNTIME_ROOT}/mem/tlab.cpp
    ${RUNTIME_ROOT}/mem/heap_manager.cpp
    ${RUNTIME_ROOT}/mem/heap_verifier.cpp
    ${RUNTIME_ROOT}/mem/rendezvous.cpp
    ${RUNTIME_ROOT}/mem/runslots.cpp
    ${RUNTIME_ROOT}/mem/region_space.cpp
    ${RUNTIME_ROOT}/mem/object_helpers.cpp
    ${RUNTIME_ROOT}/mem/mem_stats_additional_info.cpp
    ${RUNTIME_ROOT}/mem/mem_stats_default.cpp
    ${RUNTIME_ROOT}/mem/mem_stats.cpp
    ${RUNTIME_ROOT}/mem/internal_allocator.cpp
    ${RUNTIME_ROOT}/mem/panda_string.cpp
    ${RUNTIME_ROOT}/mem/memory_manager.cpp
    ${RUNTIME_ROOT}/mem/heap_space.cpp
    ${RUNTIME_ROOT}/methodtrace/trace.cpp
    ${RUNTIME_ROOT}/mark_word.cpp
    ${RUNTIME_ROOT}/method.cpp
    ${RUNTIME_ROOT}/monitor.cpp
    ${RUNTIME_ROOT}/monitor_object_lock.cpp
    ${RUNTIME_ROOT}/monitor_pool.cpp
    ${RUNTIME_ROOT}/global_object_lock.cpp
    ${RUNTIME_ROOT}/object_header.cpp
    ${RUNTIME_ROOT}/runtime.cpp
    ${RUNTIME_ROOT}/runtime_controller.cpp
    ${RUNTIME_ROOT}/string_table.cpp
    ${RUNTIME_ROOT}/thread.cpp
    ${RUNTIME_ROOT}/thread_proxy.cpp
    ${RUNTIME_ROOT}/mt_thread_manager.cpp
    ${RUNTIME_ROOT}/lock_order_graph.cpp
    ${RUNTIME_ROOT}/time_utils.cpp
    ${RUNTIME_ROOT}/timing.cpp
    ${RUNTIME_ROOT}/compiler.cpp
    ${RUNTIME_ROOT}/vtable_builder_base.cpp
    ${RUNTIME_ROOT}/imtable_builder.cpp
    ${RUNTIME_ROOT}/cframe.cpp
    ${RUNTIME_ROOT}/stack_walker.cpp
    ${RUNTIME_ROOT}/deoptimization.cpp
    ${RUNTIME_ROOT}/object_accessor.cpp
    ${RUNTIME_ROOT}/osr.cpp
    ${RUNTIME_ROOT}/arch/asm_support.cpp
    ${RUNTIME_ROOT}/signal_handler.cpp
    ${RUNTIME_ROOT}/profilesaver/profile_saver.cpp
    ${RUNTIME_ROOT}/profilesaver/profile_dump_info.cpp
    ${RUNTIME_ROOT}/jit/libprofile/pgo_file_builder.cpp
    ${RUNTIME_ROOT}/jit/profiling_loader.cpp
    ${RUNTIME_ROOT}/jit/profiling_saver.cpp
    ${RUNTIME_ROOT}/cha.cpp
    ${RUNTIME_ROOT}/runtime_helpers.cpp
    ${RUNTIME_ROOT}/handle_scope.cpp
    ${RUNTIME_ROOT}/file_manager.cpp
    ${RUNTIME_ROOT}/mem/gc/generational-gc-base.cpp
    ${RUNTIME_ROOT}/mem/gc/g1/g1-gc.cpp
    ${RUNTIME_ROOT}/mem/gc/g1/g1-helpers.cpp
    ${RUNTIME_ROOT}/mem/gc/g1/ref_updater.cpp
    ${RUNTIME_ROOT}/mem/gc/g1/g1-allocator.cpp
    ${RUNTIME_ROOT}/mem/gc/g1/g1_analytics.cpp
    ${RUNTIME_ROOT}/plugins.cpp
    ${RUNTIME_ROOT}/mem/gc/g1/update_remset_worker.cpp
    ${RUNTIME_ROOT}/mem/gc/g1/update_remset_thread.cpp
    ${RUNTIME_ROOT}/mem/gc/g1/update_remset_task_queue.cpp
    ${RUNTIME_ROOT}/mem/gc/g1/g1_pause_tracker.cpp
    ${RUNTIME_ROOT}/mem/gc/g1/hot_cards.cpp
    ${RUNTIME_ROOT}/loadable_agent.cpp
    ${RUNTIME_ROOT}/default_debugger_agent.cpp
    # ECMAScript regex engine (hand-rolled, no PCRE2 dependency)
    ${RUNTIME_ROOT}/regexp/ecmascript/regexp_parser.cpp
    ${RUNTIME_ROOT}/regexp/ecmascript/regexp_opcode.cpp
    ${RUNTIME_ROOT}/regexp/ecmascript/regexp_executor.cpp
    ${RUNTIME_ROOT}/regexp/ecmascript/mem/dyn_chunk.cpp
    ${RUNTIME_ROOT}/coroutines/coroutine.cpp
    ${RUNTIME_ROOT}/coroutines/threaded_coroutine.cpp
    ${RUNTIME_ROOT}/coroutines/stackful_coroutine.cpp
    ${RUNTIME_ROOT}/coroutines/coroutine_manager.cpp
    ${RUNTIME_ROOT}/coroutines/coroutine_worker.cpp
    ${RUNTIME_ROOT}/coroutines/threaded_coroutine_manager.cpp
    ${RUNTIME_ROOT}/coroutines/stackful_coroutine_manager.cpp
    ${RUNTIME_ROOT}/coroutines/stackful_coroutine_worker.cpp
    ${RUNTIME_ROOT}/coroutines/coroutine_events.cpp
    ${RUNTIME_ROOT}/coroutines/coroutine_stats.cpp
    ${RUNTIME_ROOT}/coroutines/priority_queue.cpp
    ${RUNTIME_ROOT}/fibers/fiber_context.cpp
    ${RUNTIME_ROOT}/compiler_thread_pool_worker.cpp
    ${RUNTIME_ROOT}/compiler_task_manager_worker.cpp
    # --- Core VM sources ---
    ${RUNTIME_ROOT}/core/core_vm.cpp
    ${RUNTIME_ROOT}/core/core_class_linker_extension.cpp
    ${RUNTIME_ROOT}/core/core_language_context.cpp
    # --- Verification sources ---
    ${VERIFICATION_ROOT}/default_plugin.cpp
    ${VERIFICATION_ROOT}/plugins.cpp
    ${VERIFICATION_ROOT}/verification_options.cpp
    ${VERIFICATION_ROOT}/verifier_messages_data.cpp
    ${VERIFICATION_ROOT}/public.cpp
    ${VERIFICATION_ROOT}/type/type_system.cpp
    ${VERIFICATION_ROOT}/type/type_type.cpp
    ${VERIFICATION_ROOT}/cflow/cflow_info.cpp
    ${VERIFICATION_ROOT}/cflow/cflow_check.cpp
    ${VERIFICATION_ROOT}/cflow/cflow_common.cpp
    ${VERIFICATION_ROOT}/absint/absint.cpp
    ${VERIFICATION_ROOT}/absint/abs_int_inl.cpp
    ${VERIFICATION_ROOT}/config/config_load.cpp
    ${VERIFICATION_ROOT}/config/context/context.cpp
    ${VERIFICATION_ROOT}/config/debug_breakpoint/breakpoint.cpp
    ${VERIFICATION_ROOT}/config/debug_breakpoint/config_handler_breakpoints.cpp
    ${VERIFICATION_ROOT}/config/default/default_config.cpp
    ${VERIFICATION_ROOT}/config/handlers/config_handler_whitelist.cpp
    ${VERIFICATION_ROOT}/config/handlers/config_handler_options.cpp
    ${VERIFICATION_ROOT}/config/handlers/config_handler_method_options.cpp
    ${VERIFICATION_ROOT}/config/handlers/config_handler_method_groups.cpp
    ${VERIFICATION_ROOT}/config/options/method_options.cpp
    ${VERIFICATION_ROOT}/config/parse/config_parse.cpp
    ${VERIFICATION_ROOT}/config/process/config_process.cpp
    ${VERIFICATION_ROOT}/config/whitelist/whitelist.cpp
    ${VERIFICATION_ROOT}/jobs/job.cpp
    ${VERIFICATION_ROOT}/jobs/service.cpp
    ${VERIFICATION_ROOT}/cache/results_cache.cpp
    # --- Interpreter impl (special compile flags) ---
    ${RUNTIME_ROOT}/interpreter/interpreter_impl.cpp
    # --- AARCH64 assembly ---
    ${RUNTIME_ROOT}/fibers/arch/aarch64/get.S
    ${RUNTIME_ROOT}/fibers/arch/aarch64/update.S
    ${RUNTIME_ROOT}/fibers/arch/aarch64/switch.S
    ${RUNTIME_ROOT}/bridge/arch/aarch64/interpreter_to_compiled_code_bridge_aarch64.S
    ${RUNTIME_ROOT}/bridge/arch/aarch64/compiled_code_to_interpreter_bridge_aarch64.S
    ${RUNTIME_ROOT}/bridge/arch/aarch64/compiled_code_to_interpreter_bridge_dyn_aarch64.S
    ${RUNTIME_ROOT}/bridge/arch/aarch64/interpreter_to_compiled_code_bridge_dyn_aarch64.S
    ${RUNTIME_ROOT}/bridge/arch/aarch64/compiled_code_to_runtime_bridge_aarch64.S
    ${RUNTIME_ROOT}/bridge/arch/aarch64/expand_compiled_code_args_dyn_aarch64.S
    ${RUNTIME_ROOT}/bridge/arch/aarch64/deoptimization_aarch64.S
    ${RUNTIME_ROOT}/arch/aarch64/osr_aarch64.S
    ${RUNTIME_ROOT}/arch/aarch64/interpreter_support.S
    # --- Platform sources (mobile not available, use common) ---
    ${SRC_ROOT}/platforms/common/runtime/thread.cpp
    ${SRC_ROOT}/platforms/common/runtime/verify_app_install.cpp
    # ==========================================================================
    # ETS Runtime sources (~122 .cpp + 2 .S)
    # From plugins/ets/runtime/CMakeLists.txt (excluding interop_js, OHOS, irtoc)
    # ==========================================================================
    # ETS core runtime
    ${ETS_PLUGIN_ROOT}/runtime/ani/ani_interaction_api.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ani/ani_mangle.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ani/ani_vm_api.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_annotation.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_class_linker.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_class_linker_context.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_class_linker_extension.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_exceptions.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_language_context.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_napi_env.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_native_library.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_native_library_provider.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_namespace_manager.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_namespace_manager_impl.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_platform_types.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_runtime_interface.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_vm.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_stubs.cpp
    ${ETS_PLUGIN_ROOT}/runtime/integrate/ets_ani_expo.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_itable_builder.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_vtable_builder.cpp
    # ETS intrinsics
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/escompat_Array.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/escompat_ArrayBuffer.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/escompat_TypedArrays.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/escompat_Date.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/escompat_JSON.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/escompat_RegExp.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/escompat_taskpool.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/escompat_Reflect.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/compiler_intrinsics.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/debugger_api.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_AniHelpers.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_Runtime.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_Class.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_AbcFile.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_AbcRuntimeLinker.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_Char.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_Console.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_Byte.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_Short.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_Int.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_Long.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_Double.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_Float.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_SyncPrimitives.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_ExclusiveLauncher.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_String.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_StringBuilder.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/gc_task_tracker.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_gc.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_finalization_registry.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_Promise.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_StackTrace.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_Type.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_TypeCreator.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_Method.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_Value.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_Arrays.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_core_Job.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_concurrency_Launch.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/escompat_Atomics.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/std_math.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/unsafe_memory.cpp
    # ETS intrinsics helpers
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/helpers/dtoa_helper.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/helpers/array_buffer_helper.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/helpers/ets_intrinsics_helpers.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/helpers/ets_to_string_cache.cpp
    ${ETS_PLUGIN_ROOT}/runtime/intrinsics/helpers/json_helper.cpp
    # ETS memory / registration
    ${ETS_PLUGIN_ROOT}/runtime/finalreg/finalization_registry_manager.cpp
    ${ETS_PLUGIN_ROOT}/runtime/mem/ets_reference_processor.cpp
    # ETS NAPI
    ${ETS_PLUGIN_ROOT}/runtime/napi/ets_napi_helpers.cpp
    ${ETS_PLUGIN_ROOT}/runtime/napi/ets_napi_invoke_interface.cpp
    ${ETS_PLUGIN_ROOT}/runtime/napi/ets_napi_native_interface.cpp
    ${ETS_PLUGIN_ROOT}/runtime/napi/ets_mangle.cpp
    # ETS regexp (PCRE2-backed)
    ${ETS_PLUGIN_ROOT}/runtime/regexp/regexp_executor.cpp
    ${ETS_PLUGIN_ROOT}/runtime/regexp/regexp.cpp
    ${ETS_PLUGIN_ROOT}/runtime/regexp/regexp_16.cpp
    ${ETS_PLUGIN_ROOT}/runtime/regexp/regexp_8.cpp
    # ETS coroutine + entrypoints + VM API
    ${ETS_PLUGIN_ROOT}/runtime/ets_coroutine.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_entrypoints.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_vm_api.cpp
    # ETS verification plugin
    ${ETS_PLUGIN_ROOT}/verification/ets_plugin.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_utils.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_object_state_info.cpp
    ${ETS_PLUGIN_ROOT}/runtime/ets_object_state_table.cpp
    # ETS types
    ${ETS_PLUGIN_ROOT}/runtime/types/ets_class.cpp
    ${ETS_PLUGIN_ROOT}/runtime/types/ets_field.cpp
    ${ETS_PLUGIN_ROOT}/runtime/types/ets_method.cpp
    ${ETS_PLUGIN_ROOT}/runtime/types/ets_promise.cpp
    ${ETS_PLUGIN_ROOT}/runtime/types/ets_promise_ref.cpp
    ${ETS_PLUGIN_ROOT}/runtime/types/ets_object.cpp
    ${ETS_PLUGIN_ROOT}/runtime/types/ets_sync_primitives.cpp
    ${ETS_PLUGIN_ROOT}/runtime/types/ets_string_builder.cpp
    ${ETS_PLUGIN_ROOT}/runtime/types/ets_typeapi_create.cpp
    ${ETS_PLUGIN_ROOT}/runtime/types/ets_typeapi_field.cpp
    ${ETS_PLUGIN_ROOT}/runtime/types/ets_typeapi_method.cpp
    ${ETS_PLUGIN_ROOT}/runtime/types/ets_typeapi_parameter.cpp
    ${ETS_PLUGIN_ROOT}/runtime/types/ets_waiters_list.cpp
    ${ETS_PLUGIN_ROOT}/runtime/types/ets_weak_reference.cpp
    ${ETS_PLUGIN_ROOT}/runtime/types/ets_finalizable_weak_ref.cpp
    ${ETS_PLUGIN_ROOT}/runtime/types/ets_job.cpp
    # ETS stdlib native (ICU/PCRE2-backed)
    ${ETS_PLUGIN_ROOT}/stdlib/native/init_native_methods.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/escompat/Process.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/etsstdlib.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/core/Intl.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/core/IntlCommon.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/core/IntlCollator.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/core/IntlDateTimeFormat.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/core/IntlDisplayNames.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/core/IntlFormattersCache.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/core/IntlLanguageTag.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/core/IntlListFormat.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/core/IntlLocale.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/core/IntlLocaleMatch.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/core/IntlNumberFormat.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/core/IntlNumberFormatters.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/core/IntlPluralRules.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/core/IntlRelativeTimeFormat.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/core/IntlSegmenter.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/core/IntlState.cpp
    ${ETS_PLUGIN_ROOT}/stdlib/native/core/stdlib_ani_helpers.cpp
    # ETS NAPI aarch64 assembly
    ${ETS_PLUGIN_ROOT}/runtime/napi/arch/arm64/ets_async_entry_point_aarch64.S
    ${ETS_PLUGIN_ROOT}/runtime/napi/arch/arm64/ets_napi_entry_point_aarch64.S
    # ICU init (generated — sets ICU data directory at runtime)
    ${GEN_ROOT}/runtime/include/init_icu_gen.cpp
)

add_library(arkruntime SHARED ${RUNTIME_SOURCES})

target_include_directories(arkruntime
    PUBLIC ${SRC_ROOT}
    PUBLIC ${SRC_ROOT}/runtime
    PUBLIC ${SRC_ROOT}/compiler
    PUBLIC ${SRC_ROOT}/libpandabase
    PUBLIC ${SRC_ROOT}/libpandafile
    PUBLIC ${GEN_ROOT}
    PUBLIC ${GEN_ROOT}/compiler
    PUBLIC ${GEN_ROOT}/generated
    PUBLIC ${GEN_ROOT}/runtime/include
    PUBLIC ${GEN_ROOT}/runtime/asm_defines
    PUBLIC ${GEN_ROOT}/cross_values
    PUBLIC ${GEN_ROOT}/libpandafile/include
    PUBLIC ${GEN_ROOT}/libpandabase/generated
    PUBLIC ${GEN_ROOT}/panda_gen_options
    PUBLIC ${VERIFICATION_ROOT}
    PUBLIC ${GEN_ROOT}/verification/gen/include
    PUBLIC ${SRC_ROOT}/plugins/ets
    PUBLIC ${SRC_ROOT}/plugins/ets/runtime
    PUBLIC ${SRC_ROOT}/plugins/ets/runtime/ani
    # ICU cross-compiled STATIC (stubdata + hmicuuc.z + hmicui18n.z)
    PUBLIC ${THIRD_PARTY}/icu/icu4c/source/common
    PUBLIC ${THIRD_PARTY}/icu/icu4c/source/i18n
    PUBLIC ${THIRD_PARTY}/icu/icu4c/source
    PUBLIC ${THIRD_PARTY}/icu
    # PCRE2 cross-compiled STATIC (pcre2-8 + pcre2-16)
    PUBLIC ${PCRE2_SRC_DIR}
    PUBLIC ${PCRE2_BIN_DIR}
)

# Interpreter impl needs global register variables on ARM64
set_source_files_properties(
    ${RUNTIME_ROOT}/interpreter/interpreter_impl.cpp
    PROPERTIES COMPILE_FLAGS "-ffixed-x20 -ffixed-x21 -ffixed-x22 -ffixed-x23 -ffixed-x24 -ffixed-x25 -ffixed-x28"
)
target_compile_definitions(arkruntime PRIVATE FFIXED_REGISTERS)

target_compile_options(arkruntime PRIVATE -Wno-invalid-offsetof)
target_compile_options(arkruntime PRIVATE -Wno-shadow -Wno-deprecated-declarations)

# ICU + PCRE2 provide all symbols; no unresolved-symbols workaround needed.
# NDK toolchain adds --no-undefined for SHARED libs.
target_link_libraries(arkruntime
    pandabase
    pandafile
    arkcompiler
    dprof
    arkaotmanager
    arktarget_options
    arkassembler
    hmicui18n.z
    hmicuuc.z
    stubdata
    pcre2-8
    pcre2-16
    atomic
    log
)

# ==============================================================================
# 16. ace_core_android (STATIC) — ACE engine core + Android adapter (Phase 3b)
# ==============================================================================
# Step A: Minimal compilation subset for Android. Does NOT include full
# ace_engine source tree; only the adapter code + essential base files.
# Referenced ace_engine sources live in ../.temp/ace_engine (upstream).
# ==============================================================================
set(ACE_ENGINE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/ace_engine)

set(ACE_ADAPTER_ANDROID_DIR ${CMAKE_CURRENT_SOURCE_DIR}/adapter/android)

set(ACE_CORE_ANDROID_SOURCES
    # --- adapter/android/entrance ---
    ${ACE_ADAPTER_ANDROID_DIR}/entrance/ace_container_android.cpp
    ${ACE_ADAPTER_ANDROID_DIR}/entrance/ace_view_android.cpp
    ${ACE_ADAPTER_ANDROID_DIR}/entrance/platform_window_android.cpp

    # --- adapter/android/osal ---
    ${ACE_ADAPTER_ANDROID_DIR}/osal/task_executor_android.cpp
    ${ACE_ADAPTER_ANDROID_DIR}/osal/asset_provider_android.cpp
    ${ACE_ADAPTER_ANDROID_DIR}/osal/platform_linker_stubs.cpp

    # --- adapter/android/stub ---
    ${ACE_ADAPTER_ANDROID_DIR}/stub/clipboard_stub.cpp
    ${ACE_ADAPTER_ANDROID_DIR}/stub/text_input_stub.cpp
    ${ACE_ADAPTER_ANDROID_DIR}/stub/event_dispatcher_stub.cpp

    # --- ace_engine frameworks/base: geometry ---
    ${ACE_ENGINE_ROOT}/frameworks/base/geometry/dimension.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/geometry/matrix4.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/geometry/matrix3.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/geometry/quaternion.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/geometry/transform_util.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/geometry/animatable_dimension.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/geometry/animatable_matrix4.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/geometry/arc_round.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/geometry/least_square_impl.cpp

    # --- ace_engine frameworks/base: utils ---
    ${ACE_ENGINE_ROOT}/frameworks/base/utils/utils.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/utils/string_utils.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/utils/utf.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/utils/date_util.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/utils/time_util.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/utils/lifecycle_checkable.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/utils/base_id.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/utils/resource_configuration.cpp

    # --- ace_engine frameworks/base: resource ---
    ${ACE_ENGINE_ROOT}/frameworks/base/resource/ace_res_config.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/resource/ace_res_key_parser.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/resource/shared_image_manager.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/resource/data_provider_manager.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/resource/internal_resource.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/resource/internal_resource_previewer.cpp

    # --- ace_engine frameworks/base: memory ---
    ${ACE_ENGINE_ROOT}/frameworks/base/memory/memory_monitor.cpp

    # --- ace_engine frameworks/base: i18n ---
    ${ACE_ENGINE_ROOT}/frameworks/base/i18n/localization.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/i18n/date_time_sequence.cpp

    # --- ace_engine frameworks/core/common ---
    ${ACE_ENGINE_ROOT}/frameworks/core/common/ace_application_info.cpp
    ${ACE_ENGINE_ROOT}/frameworks/core/common/ace_engine.cpp
    ${ACE_ENGINE_ROOT}/frameworks/core/common/container.cpp
    ${ACE_ENGINE_ROOT}/frameworks/core/common/container_scope.cpp
    ${ACE_ENGINE_ROOT}/frameworks/core/common/frontend.cpp
    ${ACE_ENGINE_ROOT}/frameworks/core/common/window.cpp
    ${ACE_ENGINE_ROOT}/frameworks/core/common/task_executor_impl.cpp
    # Preview version (no OHOS EventHandler dependency)
    ${ACE_ENGINE_ROOT}/adapter/preview/osal/task/task_runner_adapter_impl.cpp
    ${ACE_ENGINE_ROOT}/frameworks/core/common/task_runner_adapter_factory.cpp
    ${ACE_ENGINE_ROOT}/frameworks/core/common/task_runners.cpp
    ${ACE_ENGINE_ROOT}/frameworks/core/common/thread_model_impl.cpp
    ${ACE_ENGINE_ROOT}/frameworks/core/common/thread_container.cpp
    ${ACE_ENGINE_ROOT}/frameworks/core/common/asset_manager_impl.cpp
    ${ACE_ENGINE_ROOT}/frameworks/core/common/platform_bridge.cpp
    ${ACE_ENGINE_ROOT}/frameworks/core/common/watch_dog.cpp

    # --- ace_engine frameworks/base: thread/json/image ---
    ${ACE_ENGINE_ROOT}/frameworks/base/thread/background_task_executor.cpp
    ${ACE_ENGINE_ROOT}/frameworks/base/json/json_util.cpp
    ${ACE_ENGINE_ROOT}/frameworks/core/image/image_cache.cpp

    # --- ace_engine pipeline ---
    ${ACE_ENGINE_ROOT}/frameworks/core/pipeline/pipeline_base.cpp

    # --- ace_engine adapter/preview (platform-specific OSAL) ---
    ${ACE_ENGINE_ROOT}/adapter/preview/osal/system_properties.cpp

    # --- ace_engine interfaces/inner_api/ace ---
    ${ACE_ENGINE_ROOT}/interfaces/inner_api/ace/ace_forward_compatibility.cpp
)

add_library(ace_core_android STATIC ${ACE_CORE_ANDROID_SOURCES})

target_include_directories(ace_core_android
    PUBLIC
        ${ACE_ENGINE_ROOT}/frameworks
        ${ACE_ENGINE_ROOT}/interfaces
        ${ACE_ENGINE_ROOT}/interfaces/inner_api/ace_kit/include
        ${ACE_ENGINE_ROOT}/adapter/preview
        ${ACE_ENGINE_ROOT}
        # GN-style includes: foundation/arkui/ace_engine → ace_engine (symlink in engine/)
        ${CMAKE_CURRENT_SOURCE_DIR}
        # adapter/android/ root (for "adapter/android/..." include style)
        ${CMAKE_CURRENT_SOURCE_DIR}
        # adapter/android/stub/ohos (hilog/log.h, iremote_object.h, event_handler.h stubs)
        ${ACE_ADAPTER_ANDROID_DIR}/stub/ohos
        # ICU (for unicode/unistr.h — same ICU as arkruntime)
        ${THIRD_PARTY}/icu/icu4c/source/common
        ${THIRD_PARTY}/icu/icu4c/source/i18n
        ${THIRD_PARTY}/icu/icu4c/source
)

target_compile_definitions(ace_core_android PRIVATE
    CROSS_PLATFORM
    NG_BUILD
    GPU_DISABLED
    ENABLE_ROSEN_BACKEND=0
    WINDOW_SCENE_SUPPORT=0
    ACE_PREVIEW
    PREVIEW
    OHOS_PLATFORM
    ANDROID_PLATFORM
    ACE_DEBUG
    ACE_EXPORT=
    ACE_FORCE_EXPORT=
)

# ace_engine uses fRTTI and fExceptions; rtti needed for AceType/DECLARE_ACE_TYPE
target_compile_options(ace_core_android PRIVATE
    -frtti
    -fexceptions
    -Wno-shadow
    -Wno-invalid-offsetof
    -Wno-reorder
    -Wno-implicit-fallthrough
    -Wno-missing-field-initializers
    -Wno-sign-compare
    -Wno-unused-parameter
)

# ==============================================================================
# 17. libhongengine_ace (SHARED) — ACE Android shared library + JNI (Phase 3b Step B)
# ==============================================================================
# Links ace_core_android + libarkruntime + EGL/GLESv2 for first frame.
# ==============================================================================

set(ACE_JNI_SOURCES
    # JNI bridge for ACE initialization from Java/Kotlin
    ${CMAKE_CURRENT_SOURCE_DIR}/ace_engine_jni.cpp
)

add_library(hongengine_ace SHARED ${ACE_JNI_SOURCES})

target_include_directories(hongengine_ace
    PUBLIC
        ${ACE_ENGINE_ROOT}/frameworks
        ${ACE_ENGINE_ROOT}/interfaces
        ${ACE_ENGINE_ROOT}
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(hongengine_ace PRIVATE
    CROSS_PLATFORM
    NG_BUILD
    GPU_DISABLED
    ENABLE_ROSEN_BACKEND=0
    WINDOW_SCENE_SUPPORT=0
    ACE_PREVIEW
    PREVIEW
    OHOS_PLATFORM
    ANDROID_PLATFORM
)

target_compile_options(hongengine_ace PRIVATE
    -frtti
    -fexceptions
    -Wno-shadow
    -Wno-invalid-offsetof
    -Wno-unused-parameter
)

target_link_libraries(hongengine_ace
    ace_core_android
    arkruntime
    pandabase
    pandafile
    arkcompiler
    dprof
    arkaotmanager
    EGL
    GLESv2
    android
    log
)
