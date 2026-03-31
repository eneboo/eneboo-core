#!/bin/bash
# build_mac_cross_dependencies.sh
#
# Comprueba e instala las dependencias necesarias para la cross-compilación
# de eneboo hacia macOS x86_64 usando osxcross + Clang.
#
# Uso:
#   sudo ./build_mac_cross_dependencies.sh [--sdk /ruta/al/MacOSX10.13.sdk.tar.xz]
#
# Si no se proporciona --sdk, el script espera que el SDK ya esté en
# osxcross/tarballs/ o en /opt/osxcross (instalación previa).

set -e

OSXCROSS_INSTALL_PREFIX="/opt/osxcross"
OSXCROSS_REPO="https://github.com/tpoechtrager/osxcross"
OSXCROSS_CLONE_DIR="/tmp/osxcross_build"
SDK_TARBALL=""
SDK_VERSION="10.13"

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
      echo "Uso: $0 [--sdk /ruta/MacOSX10.13.sdk.tar.xz] [--prefix /opt/osxcross] [--sdk-version 10.13]"
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

# build-essential: gcc, g++, binutils y libc-dev — clang los necesita como backend para enlazar
echo "  Asegurando build-essential ..."
case "$PKG_MANAGER" in
  apt) $SUDO apt-get install -y build-essential ;;
  dnf|yum) $SUDO "$PKG_MANAGER" groupinstall -y "Development Tools" ;;
  pacman) $SUDO pacman -S --noconfirm base-devel ;;
  *) warn "Instala gcc, g++, binutils y libc-dev manualmente." ;;
esac
check_cmd gcc         gcc
check_cmd g++         g++
check_cmd ld          binutils
check_cmd ar          binutils

# clang: osxcross requiere >= 3.5. Si el del sistema es inferior, instalar desde llvm.org
CLANG_MIN_MAJOR=3
CLANG_MIN_MINOR=5

install_clang_from_llvm() {
  echo "  Instalando clang mediante el script oficial de LLVM ..."
  check_cmd wget wget
  wget -qO /tmp/llvm.sh https://apt.llvm.org/llvm.sh
  chmod +x /tmp/llvm.sh
  $SUDO bash /tmp/llvm.sh || fail "Falló la instalación de LLVM via llvm.sh"

  # Crear symlinks genéricos si el binario instalado tiene sufijo de versión
  for bin in clang clang++; do
    if ! command -v "$bin" &>/dev/null; then
      local versioned
      versioned=$(ls /usr/bin/${bin}-* 2>/dev/null | sort -V | tail -1)
      if [[ -n "$versioned" ]]; then
        $SUDO ln -sf "$versioned" "/usr/bin/${bin}"
        ok "Symlink creado: ${bin} -> ${versioned}"
      fi
    fi
  done
  ok "clang instalado: $(clang --version 2>/dev/null | head -1)"
}

clang_real_bin() {
  for ver in 20 19 18 17 16 15 14 13 12 11; do
    command -v "clang-${ver}" &>/dev/null && echo "clang-${ver}" && return 0
  done
  command -v clang || true
}

clang_version_ok() {
  local bin ver major minor
  bin=$(clang_real_bin)
  [[ -z "$bin" ]] && return 1
  ver=$("$bin" --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+' | head -1)
  [[ -z "$ver" ]] && return 1
  major=$(echo "$ver" | cut -d. -f1)
  minor=$(echo "$ver" | cut -d. -f2)
  [[ "$major" -gt "$CLANG_MIN_MAJOR" ]] && return 0
  [[ "$major" -eq "$CLANG_MIN_MAJOR" && "$minor" -ge "$CLANG_MIN_MINOR" ]] && return 0
  return 1
}

if ! command -v clang &>/dev/null && ! clang_real_bin | grep -q clang; then
  warn "clang no encontrado. Instalando desde LLVM ..."
  install_clang_from_llvm
elif ! clang_version_ok; then
  CURRENT_CLANG=$(clang --version 2>/dev/null | head -1)
  warn "clang instalado (${CURRENT_CLANG}) es inferior a ${CLANG_MIN_MAJOR}.${CLANG_MIN_MINOR}. Instalando desde LLVM ..."
  install_clang_from_llvm
else
  ok "clang encontrado y versión suficiente ($(clang --version | head -1))"
fi

# Asegurar clang++ también
if ! command -v clang++ &>/dev/null; then
  warn "clang++ no encontrado. Intentando instalar clang ..."
  install_pkg clang
  command -v clang++ &>/dev/null || fail "clang++ sigue sin encontrarse."
else
  ok "clang++ encontrado ($(command -v clang++))"
fi
check_cmd make        make

# cmake: requiere >= 3.13.4; si no existe o es inferior, compilar desde fuente
CMAKE_MIN="3.13.4"
CMAKE_URL="https://github.com/Kitware/CMake/releases/download/v${CMAKE_MIN}/cmake-${CMAKE_MIN}.tar.gz"
CMAKE_TMPDIR="/tmp/cmake_build"

install_cmake_binary() {
  # Descarga el binario precompilado de cmake para Linux x86_64 (no requiere compilador C++11)
  local ARCH
  ARCH=$(uname -m)
  local BIN_URL="https://github.com/Kitware/CMake/releases/download/v${CMAKE_MIN}/cmake-${CMAKE_MIN}-Linux-${ARCH}.tar.gz"
  local BIN_DEST="${CMAKE_TMPDIR}/cmake-${CMAKE_MIN}-Linux-${ARCH}.tar.gz"

  echo "  Descargando binario precompilado de cmake ${CMAKE_MIN} para Linux ${ARCH} ..."
  mkdir -p "${CMAKE_TMPDIR}"
  check_cmd wget wget
  wget -O "${BIN_DEST}" "${BIN_URL}" \
    || fail "No se pudo descargar cmake desde ${BIN_URL}"

  echo "  Instalando en /usr/local ..."
  tar zxf "${BIN_DEST}" -C "${CMAKE_TMPDIR}"
  local EXTRACTED_DIR="${CMAKE_TMPDIR}/cmake-${CMAKE_MIN}-Linux-${ARCH}"
  $SUDO cp -r "${EXTRACTED_DIR}/bin/"*   /usr/local/bin/
  $SUDO cp -r "${EXTRACTED_DIR}/share/"* /usr/local/share/
  ok "cmake ${CMAKE_MIN} instalado en /usr/local/bin/cmake"
}

cmake_version_ok() {
  local ver
  ver=$(cmake --version 2>/dev/null | head -1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
  [[ -z "$ver" ]] && return 1
  # Comparación de versiones: devuelve 0 si ver >= CMAKE_MIN
  printf '%s\n%s\n' "${CMAKE_MIN}" "${ver}" | sort -V -C
}

if ! command -v cmake &>/dev/null; then
  warn "cmake no encontrado. Instalando binario precompilado ..."
  install_cmake_binary
elif ! cmake_version_ok; then
  CURRENT_CMAKE=$(cmake --version 2>/dev/null | head -1)
  warn "cmake instalado (${CURRENT_CMAKE}) es inferior a ${CMAKE_MIN}. Instalando binario precompilado ..."
  install_cmake_binary
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
# qmake-qt3: solo disponible en Ubuntu antiguo; en Ubuntu moderno Qt3 no está en repos.
# El proyecto compila su propio Qt3 y genera qmake internamente, así que no es obligatorio.
if command -v qmake-qt3 &>/dev/null; then
  ok "qmake-qt3 encontrado ($(command -v qmake-qt3))"
else
  warn "qmake-qt3 no encontrado. En Ubuntu moderno Qt3 no está en los repos — el build genera su propio qmake."
fi

# ── Comprobación / instalación de osxcross ───────────────────────────────────
echo ""
echo "── osxcross ────────────────────────────────────────────"

CROSS_BIN="${OSXCROSS_INSTALL_PREFIX}/bin/x86_64-apple-darwin17-clang"

check_osxcross_sdk_version() {
  # El wrapper de osxcross es un script shell que contiene la ruta al SDK
  # Buscamos la línea: OSXCROSS_SDK="MacOSX12.3.sdk" o -isysroot .../MacOSX12.3.sdk
  local wrapper="$CROSS_BIN"
  local detected
  detected=$(grep -oE 'MacOSX[0-9]+\.[0-9]+\.sdk' "$wrapper" 2>/dev/null | head -1)
  if [[ -z "$detected" ]]; then
    # Alternativa: preguntar al compilador su sysroot
    detected=$("$wrapper" -v 2>&1 | grep -oE 'MacOSX[0-9]+\.[0-9]+\.sdk' | head -1)
  fi
  echo "${detected:-desconocido}"
}

if [[ -x "$CROSS_BIN" ]]; then
  DETECTED_SDK=$(check_osxcross_sdk_version)
  EXPECTED_SDK="MacOSX${SDK_VERSION}.sdk"
  ok "osxcross instalado: $CROSS_BIN"
  ok "Versión compilador: $("$CROSS_BIN" --version 2>&1 | head -1)"
  if [[ "$DETECTED_SDK" == "$EXPECTED_SDK" ]]; then
    ok "SDK del compilador cruzado: ${DETECTED_SDK} (coincide con SDK_VERSION=${SDK_VERSION})"
  else
    warn "SDK del compilador cruzado detectado: ${DETECTED_SDK}"
    warn "SDK esperado: ${EXPECTED_SDK} (SDK_VERSION=${SDK_VERSION})"
    warn "Puede haber inconsistencia — considera borrar /opt/osxcross y relanzar el script."
  fi
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

    # Buscar el binario real de clang: preferir el versionado (clang-20, clang-14...)
    # sobre el genérico /usr/bin/clang que en Ubuntu puede ser un stub
    find_real_clang() {
      for ver in 20 19 18 17 16 15 14 13 12 11; do
        local bin
        bin=$(command -v "clang-${ver}" 2>/dev/null || true)
        if [[ -n "$bin" ]]; then
          if echo "int main(){return 0;}" | "$bin" -x c - -o /tmp/_clang_probe 2>/dev/null; then
            rm -f /tmp/_clang_probe
            echo "$bin"   # ruta absoluta
            return 0
          fi
        fi
      done
      command -v clang || true
    }
    find_real_clangpp() {
      for ver in 20 19 18 17 16 15 14 13 12 11; do
        local bin
        bin=$(command -v "clang++-${ver}" 2>/dev/null || true)
        if [[ -n "$bin" ]]; then
          echo "$bin"   # ruta absoluta
          return 0
        fi
      done
      command -v clang++ || true
    }

    CLANG_BIN=$(find_real_clang)
    CLANGPP_BIN=$(find_real_clangpp)
    [[ -n "$CLANG_BIN" ]]   || fail "clang no encontrado en PATH"
    [[ -n "$CLANGPP_BIN" ]] || fail "clang++ no encontrado en PATH"
    ok "Usando clang: ${CLANG_BIN}"

    # Sobreescribir /usr/bin/clang y /usr/bin/clang++ si son stubs
    # Los wrappers generados por osxcross llaman a 'clang' por nombre y deben encontrar el real
    for pair in "clang:${CLANG_BIN}" "clang++:${CLANGPP_BIN}"; do
      name="${pair%%:*}"
      real="${pair##*:}"
      generic="/usr/bin/${name}"
      if [[ "$real" != "$generic" ]]; then
        warn "Sobreescribiendo stub ${generic} -> ${real}"
        $SUDO ln -sf "${real}" "${generic}"
        ok "${generic} -> ${real}"
      fi
    done

    if ! echo "int main(){return 0;}" | "${CLANG_BIN}" -x c - -o /tmp/_clang_test 2>/tmp/_clang_test.err; then
      warn "clang no puede compilar. Error:"
      cat /tmp/_clang_test.err
      warn "Intentando instalar dependencias adicionales de clang ..."
      if [[ "$PKG_MANAGER" == "apt" ]]; then
        # Extraer versión mayor del nombre del binario (clang-20 → 20)
        CLANG_VER_NUM=$(basename "${CLANG_BIN}" | grep -oE '[0-9]+$' || true)
        if [[ -n "$CLANG_VER_NUM" ]]; then
          $SUDO apt-get install -y \
            "libclang-common-${CLANG_VER_NUM}-dev" \
            "libclang-rt-${CLANG_VER_NUM}-dev" \
            libc6-dev linux-libc-dev || true
        else
          $SUDO apt-get install -y libc6-dev linux-libc-dev || true
        fi
      fi
      # Reintentar
      if ! echo "int main(){return 0;}" | "${CLANG_BIN}" -x c - -o /tmp/_clang_test 2>/tmp/_clang_test.err; then
        cat /tmp/_clang_test.err
        fail "clang sigue sin poder compilar. Revisa los errores anteriores."
      fi
    fi
    rm -f /tmp/_clang_test /tmp/_clang_test.err
    ok "clang compila correctamente (${CLANG_BIN})"

    # Exportar explícitamente CC/CXX y añadir /usr/bin al frente del PATH
    # para que los subprocesos de osxcross encuentren clang por nombre y ruta
    export CC="${CLANG_BIN}"
    export CXX="${CLANGPP_BIN}"
    export PATH="/usr/bin:/usr/local/bin:${PATH}"

    UNATTENDED=1 \
      SDK_VERSION="${SDK_VERSION}" \
      OSXCROSS_INSTALL_DESTDIR="${OSXCROSS_INSTALL_PREFIX}" \
      TARGET_DIR="${OSXCROSS_INSTALL_PREFIX}" \
      bash build.sh

    if [[ -x "${OSXCROSS_INSTALL_PREFIX}/bin/x86_64-apple-darwin17-clang" ]]; then
      ok "osxcross instalado correctamente en ${OSXCROSS_INSTALL_PREFIX}"
    else
      fail "La compilación de osxcross falló. Revisa la salida anterior."
    fi
    cd - > /dev/null
  fi
fi

# ── Parchear SDK: reemplazar #pragma options align=mac68k ────────────────────
# ColorSyncDeprecated.h usa pragmas de CodeWarrior que clang rechaza como error
SDK_DIR="${OSXCROSS_INSTALL_PREFIX}/SDK/MacOSX${SDK_VERSION}.sdk"
if [[ -d "$SDK_DIR" ]]; then
  echo "── Parche SDK ColorSyncDeprecated.h ────────────────────"
  COLORSYNC_FILES=$(find "$SDK_DIR" -name "ColorSyncDeprecated.h" -not -type l 2>/dev/null)
  if [[ -n "$COLORSYNC_FILES" ]]; then
    NEED_PATCH=false
    while IFS= read -r f; do
      if grep -q '#pragma options align=mac68k' "$f" 2>/dev/null; then
        NEED_PATCH=true
        break
      fi
    done <<< "$COLORSYNC_FILES"

    if $NEED_PATCH; then
      echo "  Aplicando parche en $SDK_DIR ..."
      while IFS= read -r f; do
        sed -i \
          -e 's/#pragma options align=mac68k/#pragma pack(push, 2)/g' \
          -e 's/#pragma options align=reset/#pragma pack(pop)/g' \
          "$f" && echo "  Parcheado: $f"
      done <<< "$COLORSYNC_FILES"
      ok "Parche ColorSyncDeprecated.h aplicado"
    else
      ok "ColorSyncDeprecated.h ya parcheado (no requiere cambios)"
    fi
  else
    warn "No se encontró ColorSyncDeprecated.h en el SDK — omitiendo parche"
  fi
fi


# ── Resumen final ────────────────────────────────────────────────────────────
echo ""
echo "════════════════════════════════════════════════════════"

CROSS_OK=false
if [[ -x "${OSXCROSS_INSTALL_PREFIX}/bin/x86_64-apple-darwin17-clang" ]]; then
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
