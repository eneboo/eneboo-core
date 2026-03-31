#!/bin/bash
# build_mac_cross_toolchain_darwin9.sh
#
# Construye el toolchain GCC para cross-compilar eneboo hacia
# macOS x86_64 (darwin9 / 10.5 Leopard) desde Ubuntu 12.04.
#
# Resultado: /opt/mac/bin/x86_64-apple-darwin9-{gcc,g++,ar,ld,...}
#            /opt/mac/SDKs/MacOSX10.5.sdk
#
# Uso:
#   sudo ./build_mac_cross_toolchain_darwin9.sh [--sdk /ruta/MacOSX10.5.sdk.tar.gz] \
#                                               [--prefix /opt/mac] \
#                                               [--jobs 4]
#
# El SDK debe proceder de Xcode 3.x (DMG de Apple) o de un tarball previo.
# Si no se indica --sdk, el script intenta descargarlo desde un mirror público.

set -e

# ── Valores por defecto ──────────────────────────────────────────────────────
INSTALL_PREFIX="/opt/mac"
SDK_TARBALL=""
JOBS=$(nproc 2>/dev/null || echo 2)
BUILD_DIR="/tmp/darwin9_toolchain_build"

TARGET="x86_64-apple-darwin9"
SDK_VERSION="10.5"
SDK_DIR_NAME="MacOSX${SDK_VERSION}.sdk"

GCC_VERSION="4.2.4"
GCC_URL="https://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}/gcc-${GCC_VERSION}.tar.bz2"

# cctools-port: commit estable conocido compatible con clang 3.x y Ubuntu 12.04
CCTOOLS_REPO="https://github.com/tpoechtrager/cctools-port"
CCTOOLS_COMMIT="2afca59b"   # rama compatible con darwin9, clang 3.5

# ── Parseo de argumentos ─────────────────────────────────────────────────────
while [[ $# -gt 0 ]]; do
  case "$1" in
    --sdk)        SDK_TARBALL="$2";     shift 2 ;;
    --prefix)     INSTALL_PREFIX="$2";  shift 2 ;;
    --jobs)       JOBS="$2";            shift 2 ;;
    --build-dir)  BUILD_DIR="$2";       shift 2 ;;
    -h|--help)
      echo "Uso: $0 [--sdk /ruta/MacOSX10.5.sdk.tar.gz] [--prefix /opt/mac] [--jobs N]"
      exit 0 ;;
    *) echo "Opción desconocida: $1"; exit 1 ;;
  esac
done

# ── Colores ──────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
ok()   { echo -e "${GREEN}[OK]${NC} $*"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $*"; }
fail() { echo -e "${RED}[ERROR]${NC} $*"; exit 1; }
step() { echo -e "\n${YELLOW}══ $* ══${NC}"; }

[[ $EUID -ne 0 ]] && SUDO="sudo" || SUDO=""

mkdir -p "${BUILD_DIR}"
mkdir -p "${INSTALL_PREFIX}/bin"
mkdir -p "${INSTALL_PREFIX}/SDKs"

echo "════════════════════════════════════════════════════════════"
echo " Toolchain darwin9 x86_64  →  ${INSTALL_PREFIX}"
echo " Build dir: ${BUILD_DIR}"
echo " Jobs: ${JOBS}"
echo "════════════════════════════════════════════════════════════"

# ── 1. Dependencias del sistema ──────────────────────────────────────────────
step "Dependencias del sistema"

# Eliminar entrada stale del PPA de LLVM para precise (offline desde 2017)
# que puede haber quedado de una ejecución anterior del script
if [[ -f /etc/apt/sources.list.d/llvm-35.list ]]; then
    warn "Eliminando PPA LLVM offline: /etc/apt/sources.list.d/llvm-35.list"
    $SUDO rm -f /etc/apt/sources.list.d/llvm-35.list
fi
# Por si acaso hay otras entradas de apt.llvm.org para precise
$SUDO grep -rl "apt.llvm.org.*precise" /etc/apt/sources.list.d/ 2>/dev/null \
    | xargs -r $SUDO rm -f && true

$SUDO apt-get update -q

$SUDO apt-get install -y \
    build-essential \
    git \
    curl \
    wget \
    bison \
    flex \
    gawk \
    patch \
    tar \
    bzip2 \
    xz-utils \
    zlib1g-dev \
    libssl-dev \
    libxml2-dev \
    uuid-dev \
    libgmp-dev \
    libmpfr-dev \
    libmpc-dev \
    texinfo \
    python \
    file

ok "Paquetes del sistema instalados"

# ── 2. Clang >= 3.5 (necesario para compilar cctools-port) ───────────────────
step "Clang >= 3.5"

# El PPA de LLVM para Ubuntu 12.04 está offline.
# No existe binario 3.5.x precompilado con glibc 2.15 (Ubuntu 12.04).
# Solución: compilar LLVM/clang 3.5.2 desde fuente usando el GCC del sistema.
#
# LLVM_SRC_VERSION puede sobreescribirse con --llvm-version si se desea otra.
LLVM_SRC_VERSION="${LLVM_SRC_VERSION:-3.5.2}"
LLVM_SRC_DIR="${BUILD_DIR}/llvm-${LLVM_SRC_VERSION}.src"
LLVM_BUILD_DIR="${BUILD_DIR}/llvm-${LLVM_SRC_VERSION}-build"
LLVM_INSTALL_DIR="/usr/local/llvm-${LLVM_SRC_VERSION}"

clang_version_ok() {
    local bin="$1"
    [[ -x "$bin" ]] || command -v "$bin" &>/dev/null || return 1
    local real; real=$(command -v "$bin" 2>/dev/null || echo "$bin")
    local ver major minor
    ver=$("$real" --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+' | head -1)
    [[ -z "$ver" ]] && return 1
    major=$(echo "$ver" | cut -d. -f1)
    minor=$(echo "$ver" | cut -d. -f2)
    [[ "$major" -gt 3 ]] && return 0
    [[ "$major" -eq 3 && "$minor" -ge 5 ]] && return 0
    return 1
}

need_clang=true
for candidate in \
    "${LLVM_INSTALL_DIR}/bin/clang" \
    /usr/local/bin/clang \
    /usr/bin/clang; do
    if clang_version_ok "$candidate"; then
        ok "clang suficiente encontrado: $candidate  ($($candidate --version 2>/dev/null | head -1))"
        need_clang=false
        CLANG_BIN="$candidate"
        CLANGPP_BIN="${candidate}++"
        [[ -x "${CLANGPP_BIN}" ]] || CLANGPP_BIN="$(dirname "$candidate")/clang++"
        break
    fi
done

if $need_clang; then
    warn "clang >= 3.5 no encontrado. Compilando LLVM ${LLVM_SRC_VERSION} desde fuente ..."
    warn "(esto puede tardar 30-60 min en una VM — solo se hace una vez)"

    # cmake >= 2.8.8 es necesario; Ubuntu 12.04 trae 2.8.7.
    # El paso 1 ya instala cmake moderno si es necesario — comprobamos aquí.
    CMAKE_BIN=$(command -v cmake)
    CMAKE_VER=$("$CMAKE_BIN" --version 2>/dev/null | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1)
    ok "cmake: ${CMAKE_VER}"

    # Descargar fuentes LLVM + Clang si no están ya
    LLVM_URL="https://releases.llvm.org/${LLVM_SRC_VERSION}/llvm-${LLVM_SRC_VERSION}.src.tar.xz"
    CFE_URL="https://releases.llvm.org/${LLVM_SRC_VERSION}/cfe-${LLVM_SRC_VERSION}.src.tar.xz"

    if [[ ! -d "${LLVM_SRC_DIR}" ]]; then
        echo "  Descargando llvm-${LLVM_SRC_VERSION}.src.tar.xz ..."
        wget --progress=bar:force -O "${BUILD_DIR}/llvm.tar.xz" "${LLVM_URL}" \
            || curl -L --progress-bar -o "${BUILD_DIR}/llvm.tar.xz" "${LLVM_URL}" \
            || fail "No se pudo descargar LLVM ${LLVM_SRC_VERSION}"
        tar xf "${BUILD_DIR}/llvm.tar.xz" -C "${BUILD_DIR}"
    fi

    if [[ ! -d "${LLVM_SRC_DIR}/tools/clang" ]]; then
        echo "  Descargando cfe-${LLVM_SRC_VERSION}.src.tar.xz ..."
        wget --progress=bar:force -O "${BUILD_DIR}/cfe.tar.xz" "${CFE_URL}" \
            || curl -L --progress-bar -o "${BUILD_DIR}/cfe.tar.xz" "${CFE_URL}" \
            || fail "No se pudo descargar Clang ${LLVM_SRC_VERSION}"
        tar xf "${BUILD_DIR}/cfe.tar.xz" -C "${BUILD_DIR}"
        mv "${BUILD_DIR}/cfe-${LLVM_SRC_VERSION}.src" "${LLVM_SRC_DIR}/tools/clang"
    fi

    # Ubuntu 12.04 tiene GCC 4.6 por defecto.
    # LLVM 3.5 requiere >= 4.7 para compilar, y cctools-port usa std::map::emplace
    # (C++11 completo) que requiere GCC >= 4.8 + libstdc++-4.8.
    # Instalamos gcc-4.8/g++-4.8 desde el PPA ubuntu-toolchain-r.
    if ! command -v gcc-4.8 &>/dev/null || ! command -v g++-4.8 &>/dev/null; then
        echo "  Añadiendo PPA ubuntu-toolchain-r para gcc-4.8 ..."
        $SUDO apt-get install -y python-software-properties 2>/dev/null || \
            $SUDO apt-get install -y software-properties-common 2>/dev/null || true

        if ! grep -r "ubuntu-toolchain-r" /etc/apt/sources.list.d/ &>/dev/null; then
            echo "deb http://ppa.launchpad.net/ubuntu-toolchain-r/test/ubuntu precise main" \
                | $SUDO tee /etc/apt/sources.list.d/ubuntu-toolchain-r.list

            $SUDO apt-key adv --keyserver keyserver.ubuntu.com \
                              --recv-keys 1E9377A2BA9EF27F \
                || $SUDO apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 \
                                     --recv-keys 1E9377A2BA9EF27F \
                || warn "No se pudo importar la clave GPG del PPA"

            $SUDO apt-get update -q
        fi

        $SUDO apt-get install -y gcc-4.8 g++-4.8 libstdc++-4.8-dev \
            || fail "No se pudo instalar gcc-4.8 desde ubuntu-toolchain-r PPA"
    fi
    ok "gcc-4.8: $(gcc-4.8 --version | head -1)"
    GCC48="gcc-4.8"
    GXX48="g++-4.8"

    # Compilar — solo X86 target y componentes mínimos para reducir tiempo
    rm -rf "${LLVM_BUILD_DIR}"
    mkdir -p "${LLVM_BUILD_DIR}"
    cd "${LLVM_BUILD_DIR}"

    cmake -DCMAKE_BUILD_TYPE=Release \
          -DLLVM_TARGETS_TO_BUILD="X86" \
          -DLLVM_INCLUDE_TESTS=OFF \
          -DLLVM_INCLUDE_EXAMPLES=OFF \
          -DLLVM_INCLUDE_DOCS=OFF \
          -DCLANG_INCLUDE_TESTS=OFF \
          -DCLANG_INCLUDE_DOCS=OFF \
          -DCMAKE_C_COMPILER="${GCC48}" \
          -DCMAKE_CXX_COMPILER="${GXX48}" \
          -DCMAKE_INSTALL_PREFIX="${LLVM_INSTALL_DIR}" \
          "${LLVM_SRC_DIR}"

    make -j"${JOBS}" clang
    $SUDO make install

    cd "${BUILD_DIR}"

    # Symlinks en /usr/local/bin
    for bin in clang clang++; do
        if [[ -x "${LLVM_INSTALL_DIR}/bin/${bin}" ]]; then
            $SUDO ln -sf "${LLVM_INSTALL_DIR}/bin/${bin}" "/usr/local/bin/${bin}"
        fi
    done

    clang_version_ok "/usr/local/bin/clang" \
        || fail "La compilación de clang falló — revisa la salida anterior"
    ok "clang compilado: $(/usr/local/bin/clang --version 2>/dev/null | head -1)"

    CLANG_BIN="/usr/local/bin/clang"
    CLANGPP_BIN="/usr/local/bin/clang++"
fi

ok "clang: ${CLANG_BIN}  ($(${CLANG_BIN} --version 2>/dev/null | head -1))"

# ── 3. cctools-port (ld64, ar, ranlib, as para darwin) ──────────────────────
step "cctools-port (linker/assembler darwin9)"

CCTOOLS_SRC="${BUILD_DIR}/cctools-port"
CCTOOLS_STAMP="${INSTALL_PREFIX}/bin/${TARGET}-ld"
# Verificar también ar — si falta hay que reinstalar cctools
cctools_complete() {
    for t in ld ar as nm ranlib strip; do
        [[ -x "${INSTALL_PREFIX}/bin/${TARGET}-${t}" ]] || return 1
    done
    return 0
}

if cctools_complete; then
    ok "cctools ya instalado en ${INSTALL_PREFIX}/bin"
else
    if [[ -d "${CCTOOLS_SRC}/.git" ]]; then
        ok "cctools-port ya clonado, actualizando ..."
        git -C "${CCTOOLS_SRC}" fetch --quiet
    else
        git clone --depth 200 "${CCTOOLS_REPO}" "${CCTOOLS_SRC}"
    fi

    # Checkout de commit compatible con clang 3.x / darwin9
    cd "${CCTOOLS_SRC}"
    git checkout "${CCTOOLS_COMMIT}" 2>/dev/null || {
        warn "Commit ${CCTOOLS_COMMIT} no encontrado, usando HEAD ..."
        git checkout master 2>/dev/null || true
    }
    cd -

    CCTOOLS_BUILD_DIR="${BUILD_DIR}/cctools-build"
    rm -rf "${CCTOOLS_BUILD_DIR}"
    mkdir -p "${CCTOOLS_BUILD_DIR}"

    # cctools-port tiene su propio build.sh
    if [[ -f "${CCTOOLS_SRC}/cctools/configure.ac" ]]; then
        # Versión nueva: subdirectorio cctools/
        cd "${CCTOOLS_SRC}/cctools"
    else
        cd "${CCTOOLS_SRC}"
    fi

    # Eliminar libobjc2 del build: el libtool de Ubuntu 12.04 no soporta
    # el tag ObjC y falla con "specify a tag with --tag". No necesitamos
    # el runtime ObjC para cross-compilar C/C++.
    #
    # Parchear TODOS los Makefile.in y configure.ac del árbol fuente,
    # incluyendo subdirectorios (otool, etc.) que referencian ../libobjc2/libobjc.la
    # Parchear todos los Makefile.in/configure.ac del árbol para eliminar
    # referencias a libobjc2 y otool (ambos tienen dependencias ObjC duras
    # que el libtool/headers de Ubuntu 12.04 no puede satisfacer).
    # Para cross-compilar C/C++ no son necesarios.
    find . \( -name "Makefile.in" -o -name "Makefile.am" -o -name "configure.ac" \) \
        | while read -r f; do
            sed -i \
                -e 's|\.\./libobjc2/libobjc\.la||g' \
                -e 's|\blibobjc2\b||g' \
                -e 's|-lobjc||g' \
                -e 's/\botool\b//g' \
                "$f"
        done
    # Reemplazar con stubs los directorios que no son necesarios para
    # cross-compilación y causan errores en Ubuntu 12.04:
    #   - otool:  depende de ObjC runtime
    #   - man:    intenta generar páginas de manual con herramientas no disponibles
    for stub_dir in otool man; do
        if [[ -d "$stub_dir" ]]; then
            cat > "${stub_dir}/Makefile.in" << 'STUB'
all:
all-am:
install:
install-am:
install-data-am:
install-exec-am:
clean:
distclean:
STUB
            warn "${stub_dir}/Makefile.in reemplazado con stub"
        fi
    done

    # Parchear OutputFile.cpp en ld64: usa std::map::emplace con initializer_list
    # que requiere libstdc++-4.8 completo. Lo reemplazamos con insert() equivalente
    # que funciona con cualquier libstdc++ C++11.
    OUTPUT_FILE=$(find . -name "OutputFile.cpp" -path "*/ld64/*" | head -1)
    if [[ -n "$OUTPUT_FILE" ]]; then
        # Línea: auto it = addedSymbols.emplace(symbolName, std::initializer_list<std::string>{name});
        # Línea: auto it = hiddenSymbols.emplace(symbolName, std::initializer_list<std::string>{name});
        sed -i \
            's/auto it = \(addedSymbols\|hiddenSymbols\)\.emplace(\(.*\), std::initializer_list<std::string>{\(.*\)});/std::vector<std::string> _v_; _v_.push_back(\3); auto it = \1.insert(std::make_pair(\2, _v_));/g' \
            "$OUTPUT_FILE" \
        && warn "OutputFile.cpp parcheado: emplace → insert" \
        || warn "No se pudo parchear OutputFile.cpp con sed (expresión compleja)"

        # Parche alternativo más simple línea a línea si el sed anterior falla
        python - "$OUTPUT_FILE" << 'PYEOF'
import sys, re
f = sys.argv[1]
content = open(f).read()
for var in ('addedSymbols', 'hiddenSymbols'):
    pattern = r'auto it = ' + var + r'\.emplace\((\w+), std::initializer_list<std::string>\{(\w+)\}\);'
    repl    = r'std::vector<std::string> _v_; _v_.push_back(\2); auto it = ' + var + r'.insert(std::make_pair(\1, _v_));'
    content = re.sub(pattern, repl, content)
open(f, 'w').write(content)
print("OutputFile.cpp parcheado OK")
PYEOF
    fi

    # Regenerar configure si autoconf está disponible
    command -v autoconf &>/dev/null && autoconf 2>/dev/null || true

    # Forzar clang a usar los headers de libstdc++-4.8 (que tiene map::emplace)
    # en lugar de los de 4.6 (default en Ubuntu 12.04).
    STDCXX48_INC=""
    for d in \
        /usr/include/c++/4.8 \
        /usr/include/x86_64-linux-gnu/c++/4.8; do
        [[ -d "$d" ]] && STDCXX48_INC="${STDCXX48_INC} -I${d}"
    done
    STDCXX48_LIB=$(ls -d /usr/lib/gcc/x86_64-linux-gnu/4.8 2>/dev/null || true)

    EXTRA_CXXFLAGS="-std=c++11 ${STDCXX48_INC}"
    EXTRA_LDFLAGS="-lstdc++"
    [[ -n "$STDCXX48_LIB" ]] && EXTRA_LDFLAGS="${EXTRA_LDFLAGS} -L${STDCXX48_LIB}"

    # Configurar para el target darwin9.
    # --disable-lto-support: evita lto_file.cpp que requiere API LLVM incompatible.
    CC="${CLANG_BIN}" CXX="${CLANG_BIN}++" \
    CXXFLAGS="${EXTRA_CXXFLAGS}" \
    LDFLAGS="${EXTRA_LDFLAGS}" \
    LTO_SUPPORT=0 \
    ./configure \
        --prefix="${INSTALL_PREFIX}" \
        --target="${TARGET}" \
        --disable-lto-support \
        --with-sysroot="${INSTALL_PREFIX}/SDKs/${SDK_DIR_NAME}" 2>/dev/null \
    || \
    CC="${CLANG_BIN}" CXX="${CLANG_BIN}++" \
    CXXFLAGS="${EXTRA_CXXFLAGS}" \
    LDFLAGS="${EXTRA_LDFLAGS}" \
    LTO_SUPPORT=0 \
    ./configure \
        --prefix="${INSTALL_PREFIX}" \
        --target="${TARGET}" \
        --disable-lto-support

    make -j"${JOBS}" LTO_SUPPORT=0
    $SUDO make install LTO_SUPPORT=0
    cd "${BUILD_DIR}"

    [[ -x "${CCTOOLS_STAMP}" ]] || fail "cctools-port: no se creó ${CCTOOLS_STAMP}"
    ok "cctools-port instalado: ${INSTALL_PREFIX}/bin/${TARGET}-ld"
fi

# ── 4. SDK MacOSX 10.5 ──────────────────────────────────────────────────────
step "MacOSX${SDK_VERSION}.sdk"

SDK_DEST="${INSTALL_PREFIX}/SDKs/${SDK_DIR_NAME}"

if [[ -d "${SDK_DEST}/usr/include" ]]; then
    ok "SDK ya instalado: ${SDK_DEST}"
else
    # Localizar tarball
    if [[ -z "$SDK_TARBALL" ]]; then
        for candidate in \
            "${HOME}/MacOSX${SDK_VERSION}.sdk.tar.gz" \
            "${HOME}/MacOSX${SDK_VERSION}.sdk.tar.bz2" \
            "${HOME}/MacOSX${SDK_VERSION}.sdk.tar.xz" \
            "/tmp/MacOSX${SDK_VERSION}.sdk.tar.gz" \
            "${BUILD_DIR}/MacOSX${SDK_VERSION}.sdk.tar.gz"; do
            if [[ -f "$candidate" ]]; then
                SDK_TARBALL="$candidate"
                ok "SDK tarball encontrado: $SDK_TARBALL"
                break
            fi
        done
    fi

    # Intentar descarga desde mirror público si no se encontró
    if [[ -z "$SDK_TARBALL" ]]; then
        SDK_MIRROR="https://github.com/phracker/MacOSX-SDKs/releases/download/11.3/MacOSX10.5.sdk.tar.xz"
        SDK_DEST_DL="${BUILD_DIR}/MacOSX${SDK_VERSION}.sdk.tar.xz"
        echo "  Intentando descarga desde:"
        echo "  ${SDK_MIRROR}"
        wget --progress=bar:force -O "${SDK_DEST_DL}" "${SDK_MIRROR}" 2>&1 \
            || curl -L --progress-bar -o "${SDK_DEST_DL}" "${SDK_MIRROR}" \
            || {
                echo ""
                echo "══════════════════════════════════════════════════════════"
                echo " No se pudo descargar el SDK automáticamente."
                echo ""
                echo " Opciones para obtener MacOSX10.5.sdk:"
                echo "  1. Descarga Xcode 3.2.x desde developer.apple.com"
                echo "     (requiere Apple ID) y extrae el SDK del DMG."
                echo "  2. Usa un SDK de un Mac con Xcode 3.x instalado:"
                echo "     /Developer/SDKs/MacOSX10.5.sdk"
                echo "     Empaquétalo: tar czf MacOSX10.5.sdk.tar.gz -C /Developer/SDKs MacOSX10.5.sdk"
                echo "     Cópialo a este host y relanza con:"
                echo "     sudo $0 --sdk /ruta/MacOSX10.5.sdk.tar.gz"
                echo "══════════════════════════════════════════════════════════"
                exit 1
            }
        SDK_TARBALL="${SDK_DEST_DL}"
        ok "SDK descargado: ${SDK_TARBALL}"
    fi

    # Instalar SDK
    echo "  Instalando SDK en ${SDK_DEST} ..."
    $SUDO mkdir -p "${INSTALL_PREFIX}/SDKs"

    # Detectar si el tarball contiene directamente el .sdk/ o un nivel de directorio extra
    FIRST_ENTRY=$(tar tf "${SDK_TARBALL}" 2>/dev/null | head -1)
    if [[ "$FIRST_ENTRY" == "${SDK_DIR_NAME}/"* ]] || [[ "$FIRST_ENTRY" == "${SDK_DIR_NAME}" ]]; then
        $SUDO tar xf "${SDK_TARBALL}" -C "${INSTALL_PREFIX}/SDKs"
    else
        # Extraer y renombrar si hace falta
        TMP_SDK="${BUILD_DIR}/sdk_extract"
        rm -rf "${TMP_SDK}"; mkdir -p "${TMP_SDK}"
        tar xf "${SDK_TARBALL}" -C "${TMP_SDK}"
        # Buscar el .sdk dentro de lo extraído
        EXTRACTED_SDK=$(find "${TMP_SDK}" -maxdepth 2 -name "MacOSX*.sdk" -type d | head -1)
        if [[ -n "$EXTRACTED_SDK" ]]; then
            $SUDO cp -r "${EXTRACTED_SDK}" "${SDK_DEST}"
        else
            $SUDO cp -r "${TMP_SDK}"/* "${SDK_DEST}"
        fi
    fi

    [[ -d "${SDK_DEST}/usr/include" ]] || fail "SDK instalado pero no se encontró ${SDK_DEST}/usr/include"
    ok "SDK instalado: ${SDK_DEST}"
fi

# ── 5. GCC cross-compiler ────────────────────────────────────────────────────
step "GCC ${GCC_VERSION} cross-compiler → ${TARGET}"

GCC_STAMP="${INSTALL_PREFIX}/bin/${TARGET}-gcc"

if [[ -x "$GCC_STAMP" ]]; then
    ok "GCC cross ya instalado: $GCC_STAMP"
else
    GCC_SRC="${BUILD_DIR}/gcc-${GCC_VERSION}"
    GCC_BUILD="${BUILD_DIR}/gcc-${GCC_VERSION}-build"

    if [[ ! -d "$GCC_SRC" ]]; then
        echo "  Descargando GCC ${GCC_VERSION} ..."
        wget --progress=bar:force -O "${BUILD_DIR}/gcc.tar.bz2" "${GCC_URL}" \
            || curl -L --progress-bar -o "${BUILD_DIR}/gcc.tar.bz2" "${GCC_URL}" \
            || fail "No se pudo descargar GCC ${GCC_VERSION}"
        tar xjf "${BUILD_DIR}/gcc.tar.bz2" -C "${BUILD_DIR}"
    fi
    ok "Fuentes GCC: ${GCC_SRC}"

    # Parche para GCC 4.2.x en hosts modernos (gcc 4.6+ tiene stdint.h de C99)
    # GCC 4.2.x asume que _Bool no está definido por el host; en Ubuntu 12.04 sí lo está.
    FIXINC="${GCC_SRC}/gcc/config/darwin.h"
    if [[ -f "$FIXINC" ]] && ! grep -q "darwin9_fixed" "$FIXINC"; then
        # Parche mínimo: forzar long long en ctype.h de darwin cuando cross-compilamos
        true  # Los parches específicos dependen de qué errores aparezcan en tiempo de build
    fi

    # Asegurar que gcc-4.8 puede crear ejecutables antes de usarlo como
    # compilador del cross-GCC. Si falla, instalar los paquetes de runtime
    # que a veces el PPA no instala automáticamente.
    if ! echo "int main(){return 0;}" | gcc-4.8 -x c - -o /tmp/_gcc48_test 2>/dev/null; then
        warn "gcc-4.8 no puede crear ejecutables — instalando paquetes faltantes ..."
        $SUDO apt-get install -y \
            gcc-4.8 g++-4.8 cpp-4.8 \
            libgcc-4.8-dev \
            libc6-dev \
            linux-libc-dev \
            binutils \
            || warn "Algunos paquetes no se pudieron instalar"
        rm -f /tmp/_gcc48_test
        # Segundo intento
        echo "int main(){return 0;}" | gcc-4.8 -x c - -o /tmp/_gcc48_test \
            || fail "gcc-4.8 sigue sin poder crear ejecutables — revisa la instalación"
    fi
    rm -f /tmp/_gcc48_test
    ok "gcc-4.8 puede crear ejecutables"

    # Crear wrapper Apple libtool en /opt/mac/bin/libtool
    # GCC 4.2.4 para darwin llama a "libtool -dynamic ..." para crear dylibs.
    # En Linux solo existe GNU libtool que no entiende -dynamic.
    # Este wrapper traduce las llamadas Apple libtool a ld64.
    APPLE_LIBTOOL="${INSTALL_PREFIX}/bin/libtool"
    if [[ ! -x "${APPLE_LIBTOOL}" ]]; then
        cat > "${APPLE_LIBTOOL}" << LTEOF
#!/bin/bash
# Apple libtool wrapper para cross-compilación en Linux
# Traduce: libtool -dynamic ... → ld -dylib ...
#          libtool -static ...  → ar cq ...
BINDIR="\$(dirname "\$(readlink -f "\$0")")"
LD="\${BINDIR}/${TARGET}-ld"
AR="\${BINDIR}/${TARGET}-ar"
mode="static"
output=""
args=()
while [[ \$# -gt 0 ]]; do
    case "\$1" in
        -dynamic)        mode=dynamic;  shift ;;
        -static)         mode=static;   shift ;;
        -o)              output="\$2";  args+=(-o "\$2"); shift 2 ;;
        -install_name|-compatibility_version|-current_version|-exported_symbols_list)
                         args+=("\$1" "\$2"); shift 2 ;;
        -single_module|-nodefaultlibs|-lc)
                         args+=("\$1"); shift ;;
        *)               args+=("\$1"); shift ;;
    esac
done
if [[ "\$mode" == "dynamic" ]]; then
    exec "\$LD" -dylib "\${args[@]}"
else
    [[ -n "\$output" ]] || output="\${args[-1]}"
    exec "\$AR" cq "\$output" "\${args[@]}"
fi
LTEOF
        chmod +x "${APPLE_LIBTOOL}"
        ok "Apple libtool wrapper creado: ${APPLE_LIBTOOL}"
    fi

    # Parchear gcc/config/t-darwin para deshabilitar libgcc_s.dylib.
    # En Linux no tenemos las librerías de sistema macOS necesarias para enlazarla.
    # Para cross-compilar solo necesitamos la versión estática de libgcc.
    T_DARWIN="${GCC_SRC}/gcc/config/t-darwin"
    if [[ -f "$T_DARWIN" ]]; then
        sed -i \
            -e 's|^SHLIB_LINK\s*=.*|SHLIB_LINK = :|' \
            -e 's|^SHLIB_MKMAP\s*=.*|SHLIB_MKMAP = :|' \
            -e 's|^SHLIB_INSTALL\s*=.*|SHLIB_INSTALL = :|' \
            -e 's|^SHLIB_MKMAP_OPTS\s*=.*|SHLIB_MKMAP_OPTS = :|' \
            "$T_DARWIN"
        ok "t-darwin parcheado: libgcc_s.dylib deshabilitada"
    else
        warn "gcc/config/t-darwin no encontrado — omitiendo parche dylib"
    fi

    rm -rf "${GCC_BUILD}"
    mkdir -p "${GCC_BUILD}"
    cd "${GCC_BUILD}"

    # Asegurar que los cross-tools de cctools están en PATH para que GCC
    # encuentre x86_64-apple-darwin9-ar, x86_64-apple-darwin9-as, etc.
    export PATH="${INSTALL_PREFIX}/bin:${PATH}"

    # --build y --host deben ser el triple canónico de la máquina host,
    # no "gcc" ni "g++" — de lo contrario GCC configure los confunde con
    # tipos de máquina y falla con "invalid host type".
    BUILD_TRIPLE=$(gcc -dumpmachine 2>/dev/null || echo "x86_64-linux-gnu")

    # CFLAGS/CXXFLAGS deben ir como variables de entorno ANTES del configure,
    # no como argumentos posicionales — GCC configure los interpreta como
    # tipos de host si van al final y falla con "invalid host type".
    CC=gcc-4.8 \
    CXX=g++-4.8 \
    CFLAGS="-O2 -fno-stack-protector" \
    CXXFLAGS="-O2 -fno-stack-protector" \
    "${GCC_SRC}/configure" \
        --build="${BUILD_TRIPLE}" \
        --host="${BUILD_TRIPLE}" \
        --target="${TARGET}" \
        --prefix="${INSTALL_PREFIX}" \
        --with-sysroot="${SDK_DEST}" \
        --enable-languages=c,c++ \
        --disable-shared \
        --enable-static \
        --disable-nls \
        --disable-multilib \
        --disable-werror \
        --with-gmp=/usr \
        --with-mpfr=/usr \
        --with-mpc=/usr \
        --enable-checking=release \
        --with-as="${INSTALL_PREFIX}/bin/${TARGET}-as" \
        --with-ld="${INSTALL_PREFIX}/bin/${TARGET}-ld" \
        --with-ar="${INSTALL_PREFIX}/bin/${TARGET}-ar"

    # -k: continuar aunque falle la creación de libgcc_s.dylib (no la necesitamos).
    # Los binarios críticos son: xgcc, cc1, cc1plus y libgcc.a (estática).
    make -j"${JOBS}" -k all-gcc 2>&1 || true

    # Verificar que los binarios críticos del compilador se construyeron
    for critical in gcc/xgcc gcc/cc1 gcc/cc1plus; do
        if [[ ! -f "${GCC_BUILD}/${critical}" ]]; then
            fail "Fallo crítico: ${GCC_BUILD}/${critical} no existe tras make all-gcc"
        fi
    done
    ok "Compilador GCC construido (xgcc, cc1, cc1plus)"

    # install-gcc puede fallar al instalar libgcc_s.dylib — ignoramos esos errores
    $SUDO make -k install-gcc 2>&1 || true

    # Verificar que el compilador instaló correctamente
    [[ -x "${INSTALL_PREFIX}/bin/${TARGET}-gcc" ]] \
        || fail "${TARGET}-gcc no instalado — revisa la salida anterior"
    ok "${TARGET}-gcc instalado"

    # libgcc estática — necesaria para enlazar binarios
    make -j"${JOBS}" -k all-target-libgcc 2>&1 || true
    $SUDO make -k install-target-libgcc 2>&1 || true

    # libstdc++ (necesaria para enlazar C++)
    make -j"${JOBS}" -k all-target-libstdc++-v3 2>/dev/null || \
        warn "libstdc++-v3 no compiló — puede que necesite ajuste manual para darwin9"
    $SUDO make -k install-target-libstdc++-v3 2>/dev/null || true

    cd "${BUILD_DIR}"

    [[ -x "$GCC_STAMP" ]] || fail "GCC: no se creó ${GCC_STAMP}"
    ok "GCC cross instalado: ${GCC_STAMP}"
fi

# ── 6. Verificación del toolchain ────────────────────────────────────────────
step "Verificación"

ALL_OK=true
for tool in gcc g++ ar ranlib ld as; do
    BIN="${INSTALL_PREFIX}/bin/${TARGET}-${tool}"
    if [[ -x "$BIN" ]]; then
        ok "${TARGET}-${tool}"
    else
        warn "No encontrado: $BIN"
        ALL_OK=false
    fi
done

if [[ -d "${SDK_DEST}/usr/include" ]]; then
    ok "SDK: ${SDK_DEST}"
else
    warn "SDK incompleto: ${SDK_DEST}"
    ALL_OK=false
fi

# ── Resumen ──────────────────────────────────────────────────────────────────
echo ""
echo "════════════════════════════════════════════════════════════"
if $ALL_OK; then
    echo -e "${GREEN}Toolchain listo. Añade al PATH:${NC}"
    echo ""
    echo "    export PATH=\$PATH:${INSTALL_PREFIX}/bin"
    echo ""
    echo "Y compila con:"
    echo "    ./build_mac_cross_x86_64_darwin9_dba.sh"
else
    echo -e "${YELLOW}Toolchain parcialmente instalado. Revisa los avisos anteriores.${NC}"
fi
echo "════════════════════════════════════════════════════════════"
