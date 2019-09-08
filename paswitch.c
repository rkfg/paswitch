#include "config.h"
#include <getopt.h>
#include <pulse/pulseaudio.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef libnotify
#include <libnotify/notification.h>
#include <libnotify/notify.h>
#define ADDOPTS "n"
#else
#define ADDOPTS ""
#endif

#define UNUSED(x) (void)(x)

int stop;
int player_idx = -1;
int game_idx = -1;
int secondary_sink_idx = -1;
int secondary_src_idx = -1;
char* player_sink_name = NULL;
char* game_source_name = NULL;
char* secondary_sink_name = NULL;
char* secondary_src_name = NULL;
int info_cnt = 0;
int remap_cnt = 0;
int dump = 0;
int notify = 0;
int force = 0;

void op_success(pa_context* c, int success, void* userdata)
{
    UNUSED(c);
    UNUSED(userdata);
    if (success) {
        remap_cnt++;
    }
    info_cnt++;
}

int nprintf(const char* str, ...)
{
    int ret;
    va_list args;
    va_start(args, str);
    char buf[1024];
    ret = vsnprintf(buf, sizeof(buf), str, args);
    va_end(args);
#ifdef libnotify
    if (notify) {
        NotifyNotification* n = notify_notification_new("PASwitch", buf, NULL);
        notify_notification_show(n, NULL);
        g_object_unref(G_OBJECT(n));
    }
#endif
    printf("%s\n", buf);
    return ret;
}

void sources_list(pa_context* c, const pa_source_info* i, int eol, void* userdata)
{
    UNUSED(c);
    UNUSED(userdata);
    if (eol) {
        info_cnt++;
        return;
    }
    if (dump) {
        printf("Source: %s\n", i->name);
    }
    if (!strcmp(i->name, secondary_src_name)) {
        secondary_src_idx = i->index;
        printf("Source found\n");
    }
}

void sinks_list(pa_context* c, const pa_sink_info* i, int eol, void* userdata)
{
    UNUSED(c);
    UNUSED(userdata);
    if (eol) {
        info_cnt++;
        return;
    }
    if (dump) {
        printf("Sink: %s\n", i->name);
    }
    if (!strcmp(i->name, secondary_sink_name)) {
        secondary_sink_idx = i->index;
        printf("Sink found\n");
    }
}

void sources_output_list(pa_context* c, const pa_source_output_info* i, int eol, void* userdata)
{
    UNUSED(c);
    UNUSED(userdata);
    if (eol) {
        info_cnt++;
        return;
    }
    if (dump) {
        printf("Source output: %s\n", i->name);
    }
    if (!strcmp(i->name, game_source_name)) {
        game_idx = i->index;
        printf("Source output found\n");
    }
}

void sinks_input_list(pa_context* c, const pa_sink_input_info* i, int eol, void* userdata)
{
    UNUSED(c);
    UNUSED(userdata);
    if (eol) {
        info_cnt++;
        return;
    }
    if (dump) {
        printf("Sink input: %s\n", i->name);
    }
    if (!strcmp(i->name, player_sink_name)) {
        player_idx = i->index;
        printf("Sink input found\n");
    }
}

void connected(pa_context* c, void* userdata)
{
    UNUSED(userdata);
    if (pa_context_get_state(c) != PA_CONTEXT_READY) {
        return;
    }
    pa_context_get_sink_input_info_list(c, sinks_input_list, NULL);
    pa_context_get_source_output_info_list(c, sources_output_list, NULL);
    pa_context_get_sink_info_list(c, sinks_list, NULL);
    pa_context_get_source_info_list(c, sources_list, NULL);
}

int main(int argc, char* argv[])
{
    char c;
    while ((c = getopt(argc, argv, "i:o:k:c:dfh" ADDOPTS)) >= 0) {
        switch (c) {
        case 'i':
            player_sink_name = optarg;
            printf("Sink input: %s\n", player_sink_name);
            break;
        case 'o':
            game_source_name = optarg;
            printf("Source output: %s\n", game_source_name);
            break;
        case 'k':
            secondary_sink_name = optarg;
            printf("Target sink: %s\n", secondary_sink_name);
            break;
        case 'c':
            secondary_src_name = optarg;
            printf("Target source: %s\n", secondary_src_name);
            break;
        case 'd':
            dump = 1;
            break;
        case 'f':
            force = 1;
            break;
        case 'h':
            printf("Usage: %s\n"
                   "  -i\tsink input\n"
                   "  -o\tsource output\n"
                   "  -k\ttarget sink\n"
                   "  -c\ttarget source\n"
                   "  -d\tdump found sinks, sources, etc.\n"
#ifdef libnotify
                   "  -n\tshow desktop notifications\n"
#endif
                   "  -f\tforce switch even if one pair was not found\n"
                   "  -h\tthis help message\n",
                argv[0]);
            return 0;
#ifdef libnotify
        case 'n':
            notify = 1;
            notify_init("paswitch");
            break;
#endif
        }
    }
    if (!player_sink_name || !game_source_name || !secondary_sink_name || !secondary_src_name) {
        printf("Not all required parameters set.\n");
        return 2;
    }
    pa_mainloop* ml = pa_mainloop_new();
    pa_context* ctx = pa_context_new(pa_mainloop_get_api(ml), "Pulseaudio switch");
    pa_context_set_state_callback(ctx, connected, NULL);
    int r = pa_context_connect(ctx, NULL, PA_CONTEXT_NOFLAGS, NULL);
    if (r < 0) {
        printf("Error connecting: %d\n", r);
        return 1;
    }
    stop = 0;
    char partial[256];
    while (!stop) {
        pa_mainloop_iterate(ml, 1, NULL);
        if (info_cnt == 4) {
            if (player_idx < 0) {
                nprintf("Sink input '%s' not found\n", player_sink_name);
                stop = 1;
            }
            if (secondary_sink_idx < 0) {
                nprintf("Target sink '%s' not found\n", secondary_sink_name);
                stop = 1;
            }
            if (game_idx < 0) {
                nprintf("Source output '%s' not found\n", game_source_name);
                stop = 1;
            }
            if (secondary_src_idx < 0) {
                nprintf("Target source '%s' not found\n", secondary_src_name);
                stop = 1;
            }
            if (stop && !force) {
                break;
            } else {
                stop = 0;
            }
            if (player_idx >= 0 && secondary_sink_idx >= 0) {
                pa_context_move_sink_input_by_index(ctx, player_idx, secondary_sink_idx, op_success, NULL);
                sprintf(partial, "Partial switch success:\n%s => %s", player_sink_name, secondary_sink_name);
            } else {
                info_cnt++;
            }
            if (game_idx >= 0 && secondary_src_idx >= 0) {
                pa_context_move_source_output_by_index(ctx, game_idx, secondary_src_idx, op_success, NULL);
                sprintf(partial, "Partial switch success:\n%s => %s", game_source_name, secondary_src_name);
            } else {
                info_cnt++;
            }
        }
        if (remap_cnt == 2 || info_cnt == 6) {
            stop = 1;
        }
    }
    if (remap_cnt == 2) {
        nprintf("Successfully switched\n%s => %s\n%s => %s", player_sink_name, secondary_sink_name, game_source_name, secondary_src_name);
    }
    if (remap_cnt == 1) {
        nprintf(partial);
    }
    pa_mainloop_quit(ml, 0);
    pa_mainloop_free(ml);
#ifdef libnotify
    if (notify) {
        notify_uninit();
    }
#endif
    return 0;
}
