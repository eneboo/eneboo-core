#!/bin/bash
# build_mac_cross_dependencies.sh
#
# Comprueba e instala las dependencias necesarias para la cross-compilación
# de eneboo hacia macOS x86_64 usando osxcross + Clang.
#
# Uso:
#   sudo ./build_mac_cross_dependencies.sh [--sdk /ruta/al/MacOSX12.3.sdk.tar.xz]
#
# Si no se proporciona --sdk, el script espera que el SDK ya esté en
# osxcross/tarballs/ o en /opt/osxcross (instalación previa).

set -e

OSXCROSS_INSTALL_PREFIX="/opt/osxcross"
OSXCROSS_REPO="https://github.com/tpoechtrager/osxcross"
OSXCROSS_CLONE_DIR="/tmp/osxcross_build"
SDK_TARBALL=""
SDK_VERSION="12.3"

# ── Parseo de argumentos ────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
  case "$1" in
    --sdk)
      SDK_TARBALL="$2"
      shift 2
      ;;
    --prefix)
      OSXCROSS_INSTALL_PREFIX="$2"
      shift 2
      ;;
    --sdk-version)
      SDK_VERSION="$2"
      shift 2
      ;;
    -h|--help)
      echo "Uso: $0 [--sdk /ruta/MacOSX12.3.sdk.tar.xz] [--prefix /opt/osxcross] [--sdk-version 12.3]"
      exit 0
      ;;
    *)
      echo "Opción desconocida: $1"
      exit 1
      ;;
  esac
done

# ── Colores ─────────────────────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

ok()   { echo -e "${GREEN}[OK]${NC} $*"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $*"; }
fail() { echo -e "${RED}[ERROR]${NC} $*"; exit 1; }

echo "════════════════════════════════════════════════════════"
echo " Verificación de dependencias: macOS cross-build (x86_64)"
echo "════════════════════════════════════════════════════════"

# ── Comprobación de root ─────────────────────────────────────────────────────
if [[ $EUID -ne 0 ]]; then
  warn "No se está ejecutando como root. Se usará 'sudo' para instalar paquetes."
  SUDO="sudo"
else
  SUDO=""
fi

# ── Detectar gestor de paquetes ──────────────────────────────────────────────
if command -v apt-get &>/dev/null; then
  PKG_MANAGER="apt"
elif command -v dnf &>/dev/null; then
  PKG_MANAGER="dnf"
elif command -v yum &>/dev/null; then
  PKG_MANAGER="yum"
elif command -v pacman &>/dev/null; then
  PKG_MANAGER="pacman"
else
  warn "Gestor de paquetes no reconocido. Instala manualmente las dependencias del sistema."
  PKG_MANAGER="unknown"
fi

install_pkg() {
  local pkg="$1"
  echo "  Instalando $pkg ..."
  case "$PKG_MANAGER" in
    apt)     $SUDO apt-get install -y "$pkg" ;;
    dnf|yum) $SUDO "$PKG_MANAGER" install -y "$pkg" ;;
    pacman)  $SUDO pacman -S --noconfirm "$pkg" ;;
    *)       warn "No se pudo instalar '$pkg' automáticamente." ;;
  esac
}

check_cmd() {
  local cmd="$1"
  local pkg="${2:-$1}"
  if command -v "$cmd" &>/dev/null; then
    ok "$cmd encontrado ($(command -v "$cmd"))"
  else
    warn "$cmd no encontrado. Intentando instalar '$pkg' ..."
    install_pkg "$pkg"
    command -v "$cmd" &>/dev/null || fail "$cmd sigue sin encontrarse tras instalar '$pkg'."
    ok "$cmd instalado."
  fi
}

# ── Dependencias del sistema necesarias para compilar osxcross ──────────────
echo ""
echo "── Dependencias del sistema ────────────────────────────"

check_cmd git         git
check_cmd clang       clang
check_cmd clang++     clang
check_cmd make        make

# cmake: requiere >= 3.13.4; si no existe o es inferior, compilar desde fuente
CMAKE_MIN="3.13.4"
CMAKE_URL="https://github.com/Kitware/CMake/releases/download/v${CMAKE_MIN}/cmake-${CMAKE_MIN}.tar.gz"
CMAKE_TMPDIR="/tmp/cmake_build"

install_cmake_from_source() {
  echo "  Descargando cmake ${CMAKE_MIN} desde fuente ..."
  mkdir -p "${CMAKE_TMPDIR}"
  check_cmd wget wget
  wget -O "${CMAKE_TMPDIR}/cmake-${CMAKE_MIN}.tar.gz" "${CMAKE_URL}" \
    || fail "No se pudo descargar cmake desde ${CMAKE_URL}"
  echo "  Extrayendo ..."
  tar zxf "${CMAKE_TMPDIR}/cmake-${CMAKE_MIN}.tar.gz" -C "${CMAKE_TMPDIR}"
  echo "  Compilando cmake (puede tardar varios minutos) ..."
  cd "${CMAKE_TMPDIR}/cmake-${CMAKE_MIN}"
  # Determinar compilador C++ disponible para el bootstrap
  if command -v g++ &>/dev/null; then
    CMAKE_CXX=g++
  elif command -v clang++ &>/dev/null; then
    CMAKE_CXX=clang++
  else
    fail "No se encuentra g++ ni clang++ para compilar cmake."
  fi
  $SUDO env CXX=$CMAKE_CXX ./bootstrap || fail "cmake bootstrap falló"
  $SUDO make        || fail "cmake make falló"
  $SUDO make install || fail "cmake make install falló"
  cd - > /dev/null
  ok "cmake ${CMAKE_MIN} instalado desde fuente."
}

cmake_version_ok() {
  local ver
  ver=$(cmake --version 2>/dev/null | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
  [[ -z "$ver" ]] && return 1
  # Comparación de versiones: devuelve 0 si ver >= CMAKE_MIN
  printf '%s\n%s\n' "${CMAKE_MIN}" "${ver}" | sort -V -C
}

if ! command -v cmake &>/dev/null; then
  warn "cmake no encontrado. Instalando desde fuente ..."
  install_cmake_from_source
elif ! cmake_version_ok; then
  CURRENT_CMAKE=$(cmake --version 2>/dev/null | head -1)
  warn "cmake instalado (${CURRENT_CMAKE}) es inferior a ${CMAKE_MIN}. Instalando desde fuente ..."
  install_cmake_from_source
else
  ok "cmake encontrado y versión suficiente ($(cmake --version | head -1))"
fi
check_cmd patch       patch
check_cmd tar         tar
check_cmd xz          xz-utils     # Debian/Ubuntu; en Fedora es 'xz'
check_cmd bzip2       bzip2
check_cmd python3     python3
check_cmd lzma        xz-utils

# libxml2 dev (necesario para osxcross cctools)
if pkg-config --exists libxml-2.0 2>/dev/null; then
  ok "libxml2-dev encontrado"
else
  warn "libxml2-dev no encontrado. Intentando instalar ..."
  case "$PKG_MANAGER" in
    apt)     install_pkg libxml2-dev ;;
    dnf|yum) install_pkg libxml2-devel ;;
    pacman)  install_pkg libxml2 ;;
    *)       warn "Instala libxml2-dev manualmente." ;;
  esac
fi

# uuid-dev (necesario para cctools-port)
if pkg-config --exists uuid 2>/dev/null; then
  ok "uuid-dev encontrado"
else
  warn "uuid-dev no encontrado. Intentando instalar ..."
  case "$PKG_MANAGER" in
    apt)     install_pkg uuid-dev ;;
    dnf|yum) install_pkg libuuid-devel ;;
    pacman)  install_pkg util-linux-libs ;;
    *)       warn "Instala uuid-dev manualmente." ;;
  esac
fi

# openssl dev (necesario para algunos módulos de osxcross)
if pkg-config --exists openssl 2>/dev/null; then
  ok "libssl-dev encontrado"
else
  warn "libssl-dev no encontrado. Intentando instalar ..."
  case "$PKG_MANAGER" in
    apt)     install_pkg libssl-dev ;;
    dnf|yum) install_pkg openssl-devel ;;
    pacman)  install_pkg openssl ;;
    *)       warn "Instala libssl-dev manualmente." ;;
  esac
fi

# ── Dependencias de build de eneboo (host) ───────────────────────────────────
echo ""
echo "── Dependencias de build del host (eneboo) ─────────────"

check_cmd g++          g++
check_cmd flex         flex
check_cmd yacc         bison        # yacc suele ser bison
check_cmd qmake-qt3    qt3-dev-tools 2>/dev/null || warn "qmake-qt3 no encontrado (necesario para QSA configure2)"

# ── Comprobación / instalación de osxcross ───────────────────────────────────
echo ""
echo "── osxcross ────────────────────────────────────────────"

CROSS_BIN="${OSXCROSS_INSTALL_PREFIX}/bin/x86_64-apple-darwin21-clang"

if [[ -x "$CROSS_BIN" ]]; then
  ok "osxcross ya instalado: $CROSS_BIN"
  ok "Versión: $("$CROSS_BIN" --version 2>&1 | head -1)"
else
  warn "osxcross no encontrado en ${OSXCROSS_INSTALL_PREFIX}."

  # Localizar el SDK tarball
  if [[ -z "$SDK_TARBALL" ]]; then
    # Buscar en ubicaciones comunes
    for candidate in \
        "${OSXCROSS_CLONE_DIR}/tarballs/MacOSX${SDK_VERSION}.sdk.tar.xz" \
        "${HOME}/MacOSX${SDK_VERSION}.sdk.tar.xz" \
        "/tmp/MacOSX${SDK_VERSION}.sdk.tar.xz"; do
      if [[ -f "$candidate" ]]; then
        SDK_TARBALL="$candidate"
        ok "SDK tarball encontrado: $SDK_TARBALL"
        break
      fi
    done
  fi

  if [[ -z "$SDK_TARBALL" ]]; then
    SDK_URL="https://github.com/joseluisq/macosx-sdks/releases/download/${SDK_VERSION}/MacOSX${SDK_VERSION}.sdk.tar.xz"
    SDK_DEST="/tmp/MacOSX${SDK_VERSION}.sdk.tar.xz"

    echo "  Descargando SDK de macOS ${SDK_VERSION} desde:"
    echo "  ${SDK_URL}"

    check_cmd curl curl
    curl -L --fail --progress-bar -o "$SDK_DEST" "$SDK_URL" \
      || fail "No se pudo descargar el SDK. Comprueba la conexión o descárgalo manualmente."

    SDK_TARBALL="$SDK_DEST"
    ok "SDK descargado en ${SDK_DEST}"
  fi

  if [[ -n "$SDK_TARBALL" ]]; then
    echo "  Clonando osxcross en ${OSXCROSS_CLONE_DIR} ..."
    if [[ -d "${OSXCROSS_CLONE_DIR}/.git" ]]; then
      ok "osxcross ya clonado, actualizando ..."
      (cd "${OSXCROSS_CLONE_DIR}" && git pull --ff-only)
    else
      git clone "${OSXCROSS_REPO}" "${OSXCROSS_CLONE_DIR}"
    fi

    mkdir -p "${OSXCROSS_CLONE_DIR}/tarballs"
    SDK_DEST_PATH="${OSXCROSS_CLONE_DIR}/tarballs/$(basename "$SDK_TARBALL")"
    if [[ "$(realpath "$SDK_TARBALL")" != "$(realpath "$SDK_DEST_PATH" 2>/dev/null)" ]]; then
      echo "  Copiando SDK tarball a ${OSXCROSS_CLONE_DIR}/tarballs/ ..."
      cp -v "$SDK_TARBALL" "${OSXCROSS_CLONE_DIR}/tarballs/"
    else
      ok "SDK tarball ya en destino, no es necesario copiar."
    fi

    echo "  Compilando osxcross (esto puede tardar varios minutos) ..."
    cd "${OSXCROSS_CLONE_DIR}"
    SDK_VERSION="${SDK_VERSION}" OSXCROSS_INSTALL_DESTDIR="${OSXCROSS_INSTALL_PREFIX}" \
      TARGET_DIR="${OSXCROSS_INSTALL_PREFIX}" bash build.sh

    if [[ -x "${OSXCROSS_INSTALL_PREFIX}/bin/x86_64-apple-darwin21-clang" ]]; then
      ok "osxcross instalado correctamente en ${OSXCROSS_INSTALL_PREFIX}"
    else
      fail "La compilación de osxcross falló. Revisa la salida anterior."
    fi
    cd - > /dev/null
  fi
fi

# ── Resumen final ────────────────────────────────────────────────────────────
echo ""
echo "════════════════════════════════════════════════════════"

CROSS_OK=false
if [[ -x "${OSXCROSS_INSTALL_PREFIX}/bin/x86_64-apple-darwin21-clang" ]]; then
  CROSS_OK=true
fi

if $CROSS_OK; then
  echo -e "${GREEN}Todo listo. Puedes compilar con:${NC}"
  echo ""
  echo "    ./build_mac_cross_x86_64_quick.sh"
  echo "    ./build_mac_cross_x86_64_dba.sh"
else
  echo -e "${YELLOW}Dependencias del sistema instaladas.${NC}"
  echo -e "${YELLOW}Falta: osxcross (necesitas el SDK de macOS — ver instrucciones arriba).${NC}"
fi
echo ""
