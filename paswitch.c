#include <getopt.h>
#include <pulse/pulseaudio.h>
#include <stdio.h>
#include <string.h>

int stop;
int player_idx = -1;
int game_idx = -1;
int secondary_sink_idx = -1;
int secondary_src_idx = -1;
char player_sink_name[256];
char game_source_name[256];
char secondary_sink_name[256];
char secondary_src_name[256];
int info_cnt = 0;
int remap_cnt = 0;
int dump = 0;

void op_success(pa_context* c, int success, void* userdata)
{
    if (success) {
        remap_cnt++;
    }
}

void sources_list(pa_context* c, const pa_source_info* i, int eol, void* userdata)
{
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
    memset(player_sink_name, 0, sizeof(player_sink_name));
    memset(game_source_name, 0, sizeof(game_source_name));
    memset(secondary_sink_name, 0, sizeof(secondary_sink_name));
    memset(secondary_src_name, 0, sizeof(secondary_src_name));
    while ((c = getopt(argc, argv, "i:o:k:c:d")) >= 0) {
        switch (c) {
        case 'i':
            strncpy(player_sink_name, optarg, sizeof(player_sink_name) - 1);
            printf("Sink input: %s\n", player_sink_name);
            break;
        case 'o':
            strncpy(game_source_name, optarg, sizeof(game_source_name) - 1);
            printf("Source output: %s\n", game_source_name);
            break;
        case 'k':
            strncpy(secondary_sink_name, optarg, sizeof(secondary_sink_name) - 1);
            printf("Target sink: %s\n", secondary_sink_name);
            break;
        case 'c':
            strncpy(secondary_src_name, optarg, sizeof(secondary_src_name) - 1);
            printf("Target source: %s\n", secondary_src_name);
            break;
        case 'd':
            dump = 1;
            break;
        }
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
    while (!stop) {
        pa_mainloop_iterate(ml, 1, NULL);
        if (info_cnt == 4) {
            info_cnt++;
            if (player_idx >= 0 && secondary_sink_idx >= 0) {
                pa_context_move_sink_input_by_index(ctx, player_idx, secondary_sink_idx, op_success, NULL);
            } else {
                remap_cnt++;
            }
            if (game_idx >= 0 && secondary_src_idx >= 0) {
                pa_context_move_source_output_by_index(ctx, game_idx, secondary_src_idx, op_success, NULL);
            } else {
                remap_cnt++;
            }
        }
        if (remap_cnt == 2) {
            stop = 1;
        }
    }
    pa_mainloop_quit(ml, 0);
    pa_mainloop_free(ml);
    return 0;
}
