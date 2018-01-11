#!/usr/bin/env bash
#
# ONNC Release Tools: common setup
#
function init_release
{
  export ONNC_SRCDIR=$(getabs "$(gettop)/trunk")
  export ONNC_EXTDIR=$(getabs "$(gettop)/onncroot")
  export ONNC_BUIDIR=$(getabs "$(gettop)/build-$(basename "${ONNC_SRCDIR}")-release")
  export ONNC_TRGDIR=$(getabs "$(gettop)/install-$(basename "${ONNC_SRCDIR}")-release")
  export ONNC_PREFIX=/opt/onnc
  export ONNC_PKGDIR=${ONNC_TRGDIR}${ONNC_PREFIX}

  export MAKE=${MAKE:-make}
  case "$(platform)" in
    freebsd)
      MAKE=gmake
      ;;
  esac

  fail_panic "fail to remove previous build"  rm -rf "${ONNC_BUIDIR}"
  fail_panic "fail to create build directory" mkdir -p "${ONNC_BUIDIR}"
}

function package_release
{
  local VERSION=$(grep -Eo '[0-9]+:[0-9]+:[0-9]+' "${ONNC_PKGDIR}/VERSION" | \
                  sed 's/:/./g')_$(date "+%y%m%d-%H%M%S")
  local PKGNAME=$1
  local TARBALL=${PKGNAME}-${VERSION}.tgz

  show "creating tarball ..."
  tar zcf "${TARBALL}" -C "$(dirname "${ONNC_PKGDIR}")" "$(basename "${ONNC_PKGDIR}")"
}
