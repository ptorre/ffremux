/**
 *
 *
 */

#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>

// if output directly to alsa will need to
// initialize libavdevice.a
// void avdevice_register_all(void);

unsigned int sleep(unsigned int seconds);


int main(int argc, char **argv)
{
    av_log_set_level(AV_LOG_INFO);

    const AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL, *device_ctx = NULL;

    AVPacket *pkt = NULL;
    const char *in_filename, *out_filename;
    int ret, i;
    int stream_index = 0;

    if (argc < 3) {
        printf("usage: %s input output\n"
               "API example program to remux a media file with libavformat and libavcodec.\n"
               "The output format is guessed according to the file extension.\n"
               "\n", argv[0]);
        return 1;
    }

    in_filename  = argv[1];
    out_filename = argv[2];

    pkt = av_packet_alloc();
    if (!pkt) {
        fprintf(stderr, "Could not allocate AVPacket\n");
        return 1;
    }

    avformat_network_init();

    const char *headers = "Cookie: testcookie=snikerdoodle;";

    AVDictionary *options = NULL;
    av_dict_set(&options, "headers", headers, 0);
    av_dict_set(&options, "referer", "ptorre.dev", 0);
    av_dict_set(&options, "user_agent", "Mozilla/5.0 (X11; CrOS aarch64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/109.0.0.0 Safari/537.36", 0);

    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, NULL, &options)) < 0) {
       av_log(NULL, AV_LOG_ERROR, "Could not open input file '%s'", in_filename);
       goto end;
    }

    {
        const AVDictionaryEntry *t=NULL;
        while (t = av_dict_get(options, "", t, AV_DICT_IGNORE_SUFFIX)) {
            av_log(NULL, AV_LOG_ERROR, "Option not accepted %s: %s\n", t->key, t->value);
        }
	if (t != NULL) {
	    abort();
	}
    }

    av_dict_free(&options);

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0) {
        fprintf(stderr, "Failed to retrieve input stream information");
        goto end;
    }

    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    AVStream* in_stream = ifmt_ctx->streams[0];

    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx) {
        fprintf(stderr, "Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    av_dump_format(ofmt_ctx, 0, out_filename, 1);

    printf("ifmt_ctx->nb_streams = %d\n", ifmt_ctx->nb_streams);
    if (in_stream->codecpar->codec_type != AVMEDIA_TYPE_AUDIO)
        return -1;

    ofmt = ofmt_ctx->oformat;
    AVStream *out_stream = avformat_new_stream(ofmt_ctx, NULL);
    if (!out_stream) {
        fprintf(stderr, "Failed allocating output stream\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);

    out_stream->codecpar->codec_tag = 0;
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            fprintf(stderr, "Could not open output file '%s'", out_filename);
            goto end;
        }
    }

    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file\n");
        goto end;
    }

    /* The first packet of some streams will not have valid dts, skip it.*/
    printf("Skipping first frame: %d\n", av_read_frame(ifmt_ctx, pkt));

    while (1) {
        AVStream *in_stream, *out_stream;

        ret = av_read_frame(ifmt_ctx, pkt);
        if (ret < 0)
            break;

        in_stream  = ifmt_ctx->streams[pkt->stream_index];
        if (pkt->stream_index != 0) {
            av_packet_unref(pkt);
            continue;
        }

        out_stream = ofmt_ctx->streams[pkt->stream_index];

        /* copy packet */


        av_packet_rescale_ts(pkt, in_stream->time_base, out_stream->time_base);
        pkt->pos = -1;

        ret = av_interleaved_write_frame(ofmt_ctx, pkt);
        /* pkt is now blank (av_interleaved_write_frame() takes ownership of
         * its contents and resets pkt), so that no unreferencing is necessary.
         * This would be different if one used av_write_frame(). */
        if (ret < 0) {
            fprintf(stderr, "Error muxing packet\n");
            break;
        }
    }

    av_write_trailer(ofmt_ctx);
end:
    av_packet_free(&pkt);

    avformat_close_input(&ifmt_ctx);

    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);

    avformat_free_context(ofmt_ctx);

    avformat_network_deinit();

    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        return 1;
    }

    return 0;
}
