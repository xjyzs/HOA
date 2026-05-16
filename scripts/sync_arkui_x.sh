#!/bin/bash
# sync_arkui_x.sh — 从 ArkUI-X 源码构建产物复制所有文件到 HOA 项目
#
# 用法:
#   ./scripts/sync_arkui_x.sh                    # 复制所有产物
#   ./scripts/sync_arkui_x.sh --so-only          # 仅复制 .so 文件
#   ./scripts/sync_arkui_x.sh --abc-only         # 仅复制 systemres .abc
#   ./scripts/sync_arkui_x.sh --res-only         # 仅复制资源文件 (JAR/resources/fonts/ICU)
#   ./scripts/sync_arkui_x.sh --dry-run          # 预览但不实际复制
#
# 依赖:
#   - ArkUI-X 构建产物目录 (默认: <arkui-x-src>/out/arkui-x/aosp_clang_arm64_release/)
#   - 项目根目录 (默认: 脚本所在目录的上级)

set -euo pipefail

# ============================================================
# 路径配置
# ============================================================
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# ArkUI-X 构建产物根目录 — 需指向实际环境中的 <arkui-x-source>/out/.../aosp_clang_arm64_release
ARKUI_BUILD="${ARKUI_BUILD:-}"

# HOA 项目目标子目录
JNILIBS_DIR="$PROJECT_ROOT/app/src/main/jniLibs/arm64-v8a"
JAR_DIR="$PROJECT_ROOT/app/libs"
SYSRES_ROOT="$PROJECT_ROOT/app/src/main/assets/sys/systemres"
SYSRES_ABC_DIR="$SYSRES_ROOT/abc"
SYSRES_RES_DIR="$SYSRES_ROOT/resources"
SYSRES_FONTS_DIR="$SYSRES_ROOT/fonts"
STUB_DIR="$PROJECT_ROOT/app/src/main/assets/sys/stub/arm64-v8a"

# ============================================================
# 选项解析
# ============================================================
SO_ONLY=false
ABC_ONLY=false
RES_ONLY=false
DRY_RUN=false

for arg in "$@"; do
    case "$arg" in
        --so-only)   SO_ONLY=true ;;
        --abc-only)  ABC_ONLY=true ;;
        --res-only)  RES_ONLY=true ;;
        --dry-run)   DRY_RUN=true ;;
        --help|-h)
            echo "用法: $0 [选项]"
            echo ""
            echo "选项:"
            echo "  (无)           复制所有产物"
            echo "  --so-only      仅复制 .so 文件 (修改 C++ 后快速更新)"
            echo "  --abc-only     仅复制 systemres .abc"
            echo "  --res-only     仅复制资源文件 (JAR/resources/fonts/ICU/stub.an)"
            echo "  --dry-run      预览，不实际复制"
            echo ""
            echo "环境变量:"
            echo "  ARKUI_BUILD    ArkUI-X 构建产物根目录（必须设置）"
            exit 0
            ;;
        *)
            echo "未知选项: $arg" >&2
            echo "用法: $0 [--so-only|--abc-only|--res-only|--dry-run|--help]" >&2
            exit 1
            ;;
    esac
done

# ============================================================
# 辅助函数
# ============================================================
log()  { echo "[$(date +%H:%M:%S)] $*"; }
warn() { echo "[$(date +%H:%M:%S)] WARN: $*" >&2; }
err()  { echo "[$(date +%H:%M:%S)] ERROR: $*" >&2; }

copy_file() {
    local src="$1"
    local dst="$2"
    if [ ! -f "$src" ]; then
        warn "源文件不存在, 跳过: $src"
        return 1
    fi
    if $DRY_RUN; then
        echo "  [DRY-RUN] $src  →  $dst"
    else
        mkdir -p "$(dirname "$dst")"
        cp -v "$src" "$dst"
    fi
}

copy_dir() {
    local src_dir="$1"
    local dst_dir="$2"
    if [ ! -d "$src_dir" ]; then
        warn "源目录不存在, 跳过: $src_dir"
        return 1
    fi
    if $DRY_RUN; then
        echo "  [DRY-RUN] $src_dir/  →  $dst_dir/"
    else
        mkdir -p "$dst_dir"
        cp -rv "$src_dir/"* "$dst_dir/" 2>/dev/null || true
    fi
}

copy_so_dir() {
    local src_dir="$1"
    local count=0
    if [ ! -d "$src_dir" ]; then
        warn "源目录不存在, 跳过: $src_dir"
        return
    fi
    for so in "$src_dir"/*.so; do
        [ -f "$so" ] || continue
        local name=$(basename "$so")
        if copy_file "$so" "$JNILIBS_DIR/$name"; then
            ((count++)) || true
        fi
    done
    log "复制 $count 个 .so ($src_dir)"
}

# ============================================================
# 验证
# ============================================================
if [ ! -d "$ARKUI_BUILD" ]; then
    err "ArkUI-X 构建产物目录不存在: $ARKUI_BUILD"
    err "请设置环境变量: export ARKUI_BUILD=/path/to/arkui-x/out/.../aosp_clang_arm64_release"
    exit 1
fi

log "========================================"
log "HOA ArkUI-X 产物同步"
log "源:  $ARKUI_BUILD"
log "目标: $PROJECT_ROOT"
$DRY_RUN && log "(仅预览模式)"
log "========================================"

# ============================================================
# Section A: .so 文件 (由 --abc-only 和 --res-only 跳过)
# ============================================================
if ! $ABC_ONLY && ! $RES_ONLY; then
    log ""
    log "========== .so 文件 =========="

    # A1. 主 .so — libarkui_android.so（含 OHOS HAP 适配 Patch）
    log "--- A1: libarkui_android.so ---"
    copy_file \
        "$ARKUI_BUILD/arkui/arkui-x/libarkui_android.so" \
        "$JNILIBS_DIR/libarkui_android.so"

    # A2. ACE 渲染引擎 .so — Cross-platform UI 组件 (39 个)
    log "--- A2: ACE 渲染引擎 .so ---"
    copy_so_dir "$ARKUI_BUILD/arkui/ace_engine_cross"

    # A3. platformview .so
    log "--- A3: platformview .so ---"
    copy_file \
        "$ARKUI_BUILD/arkui/arkui_components/libplatformview.so" \
        "$JNILIBS_DIR/libplatformview.so"

    # A4. Plugin .so — ArkUI-X 跨平台插件库
    log "--- A4: Plugin .so ---"

    # hilog: OHOS 日志系统 (@ohos.hilog)
    copy_file \
        "$ARKUI_BUILD/plugins/hilog/libhilog.so" \
        "$JNILIBS_DIR/libhilog.so"

    # abilityAccessCtrl: 权限管理 (@ohos.abilityAccessCtrl)，含 Android JNI 后端
    copy_file \
        "$ARKUI_BUILD/plugins/ability_access_ctrl/libabilityaccessctrl.so" \
        "$JNILIBS_DIR/libabilityaccessctrl.so"

    # data.preferences: 键值持久化存储 (@ohos.data.preferences)
    copy_file "$ARKUI_BUILD/plugins/data/libdata_preferences.so" \
        "$JNILIBS_DIR/libdata_preferences.so"

    # net.http: HTTP 客户端 (@ohos.net.http / @kit.NetworkKit)
    copy_file "$ARKUI_BUILD/plugins/net/libnet_http.so" \
        "$JNILIBS_DIR/libnet_http.so"

    # net.connection: 网络连接状态 (@ohos.net.connection)
    copy_file "$ARKUI_BUILD/plugins/net/libnet_connection.so" \
        "$JNILIBS_DIR/libnet_connection.so"

    # net.socket: TCP/UDP socket (@ohos.net.socket)
    copy_file "$ARKUI_BUILD/plugins/net/libnet_socket.so" \
        "$JNILIBS_DIR/libnet_socket.so"

    # net.websocket: WebSocket (@ohos.net.webSocket)
    copy_file "$ARKUI_BUILD/plugins/net/libnet_websocket.so" \
        "$JNILIBS_DIR/libnet_websocket.so"

    # uri: URI 解析 (@ohos.uri)
    copy_file "$ARKUI_BUILD/plugins/uri/liburi.so" \
        "$JNILIBS_DIR/liburi.so"

    # url: URL 解析 (@ohos.url)
    copy_file "$ARKUI_BUILD/plugins/url/liburl.so" \
        "$JNILIBS_DIR/liburl.so"

    # util: 基础工具模块
    copy_file "$ARKUI_BUILD/plugins/util/libutil.so" \
        "$JNILIBS_DIR/libutil.so"

    # util.HashMap: HashMap 数据结构 (@ohos.util.HashMap)
    copy_file "$ARKUI_BUILD/plugins/util/libutil_hashmap.so" \
        "$JNILIBS_DIR/libutil_hashmap.so"

    # web.webview: WebView 组件 (@ohos.web.webview)
    copy_file "$ARKUI_BUILD/plugins/web/libweb_webview.so" \
        "$JNILIBS_DIR/libweb_webview.so"
    copy_file "$ARKUI_BUILD/plugins/web/ace_web_webview_android.jar" \
        "$JAR_DIR/ace_web_webview_android.jar"

    # ---- 以下为其他常用插件，按需取消注释 ----
    # copy_file "$ARKUI_BUILD/plugins/i18n/libi18n.so"             "$JNILIBS_DIR/libi18n.so"
    # copy_file "$ARKUI_BUILD/plugins/intl/libintl.so"             "$JNILIBS_DIR/libintl.so"
    # copy_file "$ARKUI_BUILD/plugins/process/libprocess.so"       "$JNILIBS_DIR/libprocess.so"
    # copy_file "$ARKUI_BUILD/plugins/display/libdisplay.so"       "$JNILIBS_DIR/libdisplay.so"
    # copy_file "$ARKUI_BUILD/plugins/device_info/libdeviceinfo.so" "$JNILIBS_DIR/libdeviceinfo.so"
    # copy_file "$ARKUI_BUILD/plugins/file/libfile_fs.so"          "$JNILIBS_DIR/libfile_fs.so"
    # copy_file "$ARKUI_BUILD/plugins/file/libfile_picker.so"      "$JNILIBS_DIR/libfile_picker.so"
    # copy_file "$ARKUI_BUILD/plugins/worker/libworker.so"         "$JNILIBS_DIR/libworker.so"
    # copy_file "$ARKUI_BUILD/plugins/taskpool/libtaskpool.so"     "$JNILIBS_DIR/libtaskpool.so"

    # A5. 传递依赖 — 以上插件所需的基础库
    log "--- A5: 传递依赖 ---"

    # net_utils: net.* 插件 (http/socket/websocket) 的内部依赖
    copy_file "$ARKUI_BUILD/plugins/net_utils/libnet_utils.so" \
        "$JNILIBS_DIR/libnet_utils.so"

    # curl: libnet_http.so 的依赖
    copy_file "$ARKUI_BUILD/thirdparty/curl/libcurl_shared.so" \
        "$JNILIBS_DIR/libcurl_shared.so"

    # nghttp2: libcurl_shared.so 的 HTTP/2 依赖
    copy_file "$ARKUI_BUILD/thirdparty/nghttp2/libnghttp2_shared.so" \
        "$JNILIBS_DIR/libnghttp2_shared.so"

    # openssl: HTTP/TLS 加解密 (libnet_http, libutil 等依赖)
    copy_file "$ARKUI_BUILD/thirdparty/openssl/libcrypto_openssl.so" \
        "$JNILIBS_DIR/libcrypto_openssl.so"
    copy_file "$ARKUI_BUILD/thirdparty/openssl/libssl_openssl.so" \
        "$JNILIBS_DIR/libssl_openssl.so"

    # xml2: libdata_preferences.so 的依赖
    copy_file "$ARKUI_BUILD/thirdparty/libxml2/libxml2.so" \
        "$JNILIBS_DIR/libxml2.so"

    # zlib: libcurl_shared.so 的压缩依赖
    copy_file "$ARKUI_BUILD/thirdparty/zlib/libshared_libz.so" \
        "$JNILIBS_DIR/libshared_libz.so"
fi

# ============================================================
# Section B: systemres .abc (由 --so-only 和 --res-only 跳过)
# ============================================================
if ! $SO_ONLY && ! $RES_ONLY; then
    log ""
    log "========== systemres .abc =========="
    log "--- B1: 框架 UI 系统组件 .abc ---"
    SRC_ABC_DIR="$ARKUI_BUILD/arkui/ace_engine_cross"
    if [ -d "$SRC_ABC_DIR" ]; then
        count=0
        for abc in "$SRC_ABC_DIR"/*.abc; do
            [ -f "$abc" ] || continue
            abc_name=$(basename "$abc")
            if copy_file "$abc" "$SYSRES_ABC_DIR/$abc_name"; then
                ((count++)) || true
            fi
        done
        log "复制 $count 个 .abc"
    else
        warn "systemres .abc 源目录不存在: $SRC_ABC_DIR"
    fi
fi

# ============================================================
# Section C: 资源文件 (由 --so-only 和 --abc-only 跳过)
# ============================================================
if ! $SO_ONLY && ! $ABC_ONLY; then
    log ""
    log "========== 资源文件 =========="

    # C1. arkui_android_adapter.jar — ArkUI-X Android 适配器
    log "--- C1: arkui_android_adapter.jar ---"
    copy_file \
        "$ARKUI_BUILD/arkui_android_adapter.jar" \
        "$JAR_DIR/arkui_android_adapter.jar"

    # C2. resources.index — 系统资源索引
    log "--- C2: resources.index ---"
    copy_file \
        "$ARKUI_BUILD/obj/interface/sdk/systemres/resources.index" \
        "$SYSRES_ROOT/resources.index"

    # C3. resources/ — 系统资源目录 (base/dark/wearable)
    log "--- C3: resources/ ---"
    copy_dir \
        "$ARKUI_BUILD/obj/interface/sdk/systemres/resources" \
        "$SYSRES_RES_DIR"

    # C4. fonts/ — 字体文件 (HMSymbolVF.ttf 等)
    log "--- C4: fonts/ ---"
    copy_dir \
        "$ARKUI_BUILD/obj/interface/sdk/systemres_fonts" \
        "$SYSRES_FONTS_DIR"

    # C5. ICU 国际化数据 (重命名 74l → 72l 以匹配运行时预期)
    log "--- C5: ICU 数据 ---"
    copy_file \
        "$ARKUI_BUILD/icu_data/out/icudt74l.dat" \
        "$SYSRES_ROOT/icudt72l.dat"

    # C6. stub.an — ArkTS 运行时桩文件
    log "--- C6: stub.an ---"
    copy_file \
        "$ARKUI_BUILD/gen/arkcompiler/ets_runtime/stub.an" \
        "$STUB_DIR/stub.an"
fi

# ============================================================
# 完成
# ============================================================
log ""
log "========================================"
if $DRY_RUN; then
    log "预览完成 (未实际复制)。去掉 --dry-run 执行真实复制。"
else
    log "同步完成。"
fi
log "========================================"
