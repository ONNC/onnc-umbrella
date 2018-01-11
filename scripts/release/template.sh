#!/usr/bin/env bash
#
# ONNC Release Tools: template
source ./scripts/library.sh
source ./scripts/release/common.sh

set -e
init_release

##
# Build external projects
#
ONNC_EXTSRC=${ONNC_SRCDIR}/external
show "building external libraries ..."
build_skypat     "${ONNC_EXTSRC}/SkyPat-3.0"            "${ONNC_EXTDIR}"
#build_openssl    "${ONNC_EXTSRC}/openssl-1.0.2l"        "${ONNC_EXTDIR}"
#build_libevent   "${ONNC_EXTSRC}/libevent-2.1.8-stable" "${ONNC_EXTDIR}"
#build_mdbm       "${ONNC_EXTSRC}/mdbm-4.12.4"           "${ONNC_EXTDIR}"
#build_postgresql "${ONNC_EXTSRC}/postgresql-9.6.1"      "${ONNC_EXTDIR}"
#build_llvm       "${ONNC_EXTSRC}/llvm-4.0.1"            "${ONNC_EXTDIR}"

##
# Build ONNC
#
show "building ONNC..."
pushd "${ONNC_BUIDIR}"
"${ONNC_SRCDIR}/autogen.sh"
"${ONNC_SRCDIR}/configure" --prefix="${ONNC_PREFIX}" \
                          --with-skypat="${ONNC_EXTDIR}" \
                          --enable-optimize \
                          --enable-release
"${MAKE}" DESTDIR="${ONNC_TRGDIR}" install
popd

package_release "ONNC"
