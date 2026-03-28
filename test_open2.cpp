#include <cstdio>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
}

int main() {
    avdevice_register_all();
    const AVInputFormat *fmt = av_find_input_format("avfoundation");

    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "pixel_format", "uyvy422", 0);

    AVFormatContext *ctx = nullptr;
    printf("Opening camera (no size constraint)...\n");
    fflush(stdout);
    int ret = avformat_open_input(&ctx, "0", fmt, &opts);
    av_dict_free(&opts);
    if (ret < 0) {
        char buf[256];
        av_strerror(ret, buf, sizeof(buf));
        printf("Error: %s\n", buf);
        return 1;
    }
    printf("Camera opened!\n");

    for (unsigned i = 0; i < ctx->nb_streams; i++) {
        AVCodecParameters *par = ctx->streams[i]->codecpar;
        printf("Stream %u: codec_id=%d (%s), %dx%d, pix_fmt=%d\n",
               i, par->codec_type, par->codec_id,
               avcodec_get_name(par->codec_id), par->width, par->height, par->format);
    }

    AVPacket *pkt = av_packet_alloc();
    ret = av_read_frame(ctx, pkt);
    printf("av_read_frame: %d, size=%d\n", ret, pkt->size);
    av_packet_free(&pkt);

    avformat_close_input(&ctx);
    return 0;
}
