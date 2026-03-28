#include <cstdio>
#include <dispatch/dispatch.h>
#include <CoreFoundation/CoreFoundation.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
}

int main() {
    avdevice_register_all();
    const AVInputFormat *fmt = av_find_input_format("avfoundation");

    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "framerate", "30", 0);
    av_dict_set(&opts, "pixel_format", "uyvy422", 0);

    AVFormatContext *ctx = nullptr;

    // Open camera on a background thread so main thread can run CFRunLoop
    __block int openResult = 0;
    __block AVFormatContext *localCtx = nullptr;
    __block AVDictionary *localOpts = opts;
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        printf("Opening camera from background thread...\n");
        fflush(stdout);
        openResult = avformat_open_input(&localCtx, "0", fmt, &localOpts);
        av_dict_free(&localOpts);
        printf("avformat_open_input returned: %d\n", openResult);
        fflush(stdout);
        dispatch_semaphore_signal(sem);
        CFRunLoopStop(CFRunLoopGetMain());
    });

    // Run the main run loop so AVFoundation callbacks work
    CFRunLoopRunInMode(kCFRunLoopDefaultMode, 10.0, false);
    dispatch_semaphore_wait(sem, DISPATCH_TIME_FOREVER);
    ctx = localCtx;

    if (openResult < 0) {
        char buf[256];
        av_strerror(openResult, buf, sizeof(buf));
        printf("Error: %s\n", buf);
        return 1;
    }

    printf("Camera opened!\n");
    for (unsigned i = 0; i < ctx->nb_streams; i++) {
        AVCodecParameters *par = ctx->streams[i]->codecpar;
        printf("Stream %u: type=%d, codec=%s, %dx%d, fmt=%d\n",
               i, par->codec_type, avcodec_get_name(par->codec_id),
               par->width, par->height, par->format);
    }
    fflush(stdout);

    AVPacket *pkt = av_packet_alloc();
    int ret = av_read_frame(ctx, pkt);
    printf("av_read_frame: %d, size=%d\n", ret, pkt->size);
    av_packet_free(&pkt);

    avformat_close_input(&ctx);
    return 0;
}
