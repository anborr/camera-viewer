#include <cstdio>
#include <cstdarg>
extern "C" {
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
}

static void log_cb(void *, int, const char *fmt, va_list vl) {
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, vl);
    printf("LOG: [%s]\n", buf);
}

int main() {
    avdevice_register_all();
    const AVInputFormat *fmt = av_find_input_format("avfoundation");
    av_log_set_callback(log_cb);
    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "list_devices", "true", 0);
    AVFormatContext *ctx = avformat_alloc_context();
    avformat_open_input(&ctx, "", fmt, &opts);
    av_dict_free(&opts);
    if (ctx) avformat_close_input(&ctx);
    return 0;
}
