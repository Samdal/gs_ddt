#ifndef GS_DDT_H_
#define GS_DDT_H_

typedef void(*gs_ddt_func)(int argc, char** argv);

typedef struct gs_ddt_command_s {
        gs_ddt_func func;
        const char* name;
        const char* desc;
} gs_ddt_command_t;

extern void gs_ddt_printf(const char* fmt, ...);
extern void gs_ddt_update(gs_gui_context_t* ctx, const gs_gui_selector_desc_t* desc,
                          int open, int autoscroll, float size, float open_speed, float close_speed,
                          gs_ddt_command_t* commands, int command_len);
// A gui context must be begun before this is ran

#ifdef GS_DDT_IMPL

static char ddt_tb[2048] = ""; // text buffer
static char ddt_cb[524]  = ""; // "command" buffer

void
gs_ddt_printf(const char* fmt, ...)
{
        char tmp[512] = "";
        va_list args;

        va_start(args, fmt);
        vsnprintf(tmp, sizeof(tmp), fmt, args);
        va_end(args);

        int n = sizeof(ddt_tb) - strlen(ddt_tb) - 1;
        int resize = strlen(tmp)  - n;
        if (resize > 0)
                memmove(ddt_tb, ddt_tb + resize, sizeof(ddt_tb) - resize);
        n = sizeof(ddt_tb) - strlen(ddt_tb) - 1;
        strncat(ddt_tb, tmp, n);
}

void
gs_ddt_update(gs_gui_context_t* ctx, const gs_gui_selector_desc_t* desc,
              int open, int autoscroll, float size, float open_speed, float close_speed,
              gs_ddt_command_t* commands, int command_len)
{
        gs_vec2 fb = ctx->framebuffer_size;
        static float ddty;
        static int last_open_state;

        if (open)
                ddty = gs_interp_linear(ddty, fb.y * size, open_speed);
        else if (!open && ddty > 0)
                ddty = gs_interp_linear(ddty, -1, close_speed);
        else if (!open)
                return;

        if (gs_gui_window_begin_ex(ctx, "gs_ddt_content", gs_gui_rect(0, 0, fb.x, ddty - 24), NULL, NULL,
                                   GS_GUI_OPT_FORCESETRECT | GS_GUI_OPT_NOTITLE | GS_GUI_OPT_NORESIZE)) {
                gs_gui_layout_row(ctx, 1, (int[]){-1}, 0);
                gs_gui_text(ctx, ddt_tb);
                if (autoscroll) gs_gui_get_current_container(ctx)->scroll.y = sizeof(ddt_tb)*7+100;
                gs_gui_window_end(ctx);
        }

        if (gs_gui_window_begin_ex(ctx, "gs_ddt_input", gs_gui_rect(0, ddty - 24, fb.x, 24), NULL, NULL,
                                   GS_GUI_OPT_FORCESETRECT | GS_GUI_OPT_NOTITLE | GS_GUI_OPT_NORESIZE)) {
                int len = strlen(ddt_cb);
                gs_gui_layout_row(ctx, 3, (int[]){14, len * 7+2, 10}, 0);
                gs_gui_text(ctx, "$>");
                gs_gui_text(ctx, ddt_cb);

                // handle text input
                int32_t n = gs_min(sizeof(ddt_cb) - len - 1, (int32_t) strlen(ctx->input_text));
                if (!open || !last_open_state) {

                } else if (ctx->key_pressed & GS_GUI_KEY_RETURN) {
                        gs_ddt_printf("$ %s\n", ddt_cb);

                        if (*ddt_cb && commands) {
                                char* tmp = ddt_cb;
                                int argc = 1;
                                while((tmp = strchr(tmp, ' '))) {
                                        argc++;
                                        tmp++;
                                }

                                tmp = ddt_cb;
                                char* last_pos = ddt_cb;
                                char* argv[argc];
                                int i = 0;
                                while((tmp = strchr(tmp, ' '))) {
                                        *tmp = 0;
                                        argv[i++] = last_pos;
                                        last_pos = ++tmp;
                                }
                                argv[argc-1] = last_pos;

                                for (int i = 0; i < command_len; i++) {
                                        if (commands[i].name && commands[i].func && strcmp(argv[0], commands[i].name) == 0) {
                                                commands[i].func(argc, argv);
                                                goto command_found;
                                        }
                                }
                                gs_ddt_printf("[gs_ddt]: unrecognized command '%s'\n", argv[0]);
                        command_found:
                                ddt_cb[0] = '\0';
                        }
                } else if (ctx->key_pressed & GS_GUI_KEY_BACKSPACE && len > 0) {
                        // skip utf-8 continuation bytes
                        while ((ddt_cb[--len] & 0xc0) == 0x80 && len > 0);
                        ddt_cb[len] = '\0';
                } else if (n > 0) {
                        memcpy(ddt_cb + len, ctx->input_text, n);
                        len += n;
                        ddt_cb[len] = '\0';
                }

                // blinking cursor
                gs_gui_get_layout(ctx)->body.x -= 5;
                if ((int)(gs_platform_elapsed_time() / 666.0f) & 1)
                        gs_gui_text(ctx, "|");
                gs_gui_window_end(ctx);

                last_open_state = open;
        }
}
#endif // GS_DDT_IMPL

#endif // GS_DDT_H_
