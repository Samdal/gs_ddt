#ifndef GS_DDT_H_
#define GS_DDT_H_

typedef void(*gs_ddt_func)(int argc, char** argv);

typedef struct gs_ddt_command_s {
        gs_ddt_func func;
        const char* name;
        const char* desc;
} gs_ddt_command_t;

typedef struct gs_ddt_s {
        char tb[2048]; // text buffer
        char cb[524]; // "command" buffer

        float y;
        float size;
        float open_speed;
        float close_speed;

        int open;
        int last_open_state;
        int autoscroll;

        gs_ddt_command_t* commands;
        int commands_len;
} gs_ddt_t;

extern void gs_ddt_printf(gs_ddt_t* ddt, const char* fmt, ...);
extern void gs_ddt(gs_ddt_t* ddt, gs_gui_context_t* ctx, gs_gui_rect_t screen, const gs_gui_selector_desc_t* desc);
// A gui context must be begun before this is ran

#ifdef GS_DDT_IMPL

void
gs_ddt_printf(gs_ddt_t* ddt, const char* fmt, ...)
{
        char tmp[512] = {0};
        va_list args;

        va_start(args, fmt);
        vsnprintf(tmp, sizeof(tmp), fmt, args);
        va_end(args);

        int n = sizeof(ddt->tb) - strlen(ddt->tb) - 1;
        int resize = strlen(tmp)  - n;
        if (resize > 0) {
                memmove(ddt->tb, ddt->tb + resize, sizeof(ddt->tb) - resize);
                n = sizeof(ddt->tb) - strlen(ddt->tb) - 1;
        }
        strncat(ddt->tb, tmp, n);
}

void
gs_ddt(gs_ddt_t* ddt, gs_gui_context_t* ctx, gs_gui_rect_t screen, const gs_gui_selector_desc_t* desc)
{
        if (ddt->open)
                ddt->y = gs_interp_linear(ddt->y, screen.h * ddt->size, ddt->open_speed);
        else if (!ddt->open && ddt->y > 0)
                ddt->y = gs_interp_linear(ddt->y, -1, ddt->close_speed);
        else if (!ddt->open)
                return;

        if (gs_gui_window_begin_ex(ctx, "gs_ddt_content", gs_gui_rect(screen.x, screen.y, screen.w, ddt->y - 24), NULL, NULL,
                                   GS_GUI_OPT_FORCESETRECT | GS_GUI_OPT_NOTITLE | GS_GUI_OPT_NORESIZE | GS_GUI_OPT_NODOCK)) {
                gs_gui_layout_row(ctx, 1, (int[]){-1}, 0);
                gs_gui_text(ctx, ddt->tb);
                if (ddt->autoscroll) gs_gui_get_current_container(ctx)->scroll.y = sizeof(ddt->tb)*7+100;
                gs_gui_window_end(ctx);
        }

        if (gs_gui_window_begin_ex(ctx, "gs_ddt_input", gs_gui_rect(screen.x, ddt->y - 24, screen.w, 24), NULL, NULL,
                                   GS_GUI_OPT_FORCESETRECT | GS_GUI_OPT_NOTITLE | GS_GUI_OPT_NORESIZE | GS_GUI_OPT_NODOCK)) {
                int len = strlen(ddt->cb);
                gs_gui_layout_row(ctx, 3, (int[]){14, len * 7+2, 10}, 0);
                gs_gui_text(ctx, "$>");
                gs_gui_text(ctx, ddt->cb);

                // handle text input
                int32_t n = gs_min(sizeof(ddt->cb) - len - 1, (int32_t) strlen(ctx->input_text));
                if (!ddt->open || !ddt->last_open_state) {

                } else if (ctx->key_pressed & GS_GUI_KEY_RETURN) {
                        gs_ddt_printf(ddt, "$ %s\n", ddt->cb);

                        if (*ddt->cb && ddt->commands) {
                                char* tmp = ddt->cb;
                                int argc = 1;
                                while((tmp = strchr(tmp, ' '))) {
                                        argc++;
                                        tmp++;
                                }

                                tmp = ddt->cb;
                                char* last_pos = ddt->cb;
                                char* argv[argc];
                                int i = 0;
                                while((tmp = strchr(tmp, ' '))) {
                                        *tmp = 0;
                                        argv[i++] = last_pos;
                                        last_pos = ++tmp;
                                }
                                argv[argc-1] = last_pos;

                                for (int i = 0; i < ddt->commands_len; i++) {
                                        if (ddt->commands[i].name && ddt->commands[i].func && strcmp(argv[0], ddt->commands[i].name) == 0) {
                                                ddt->commands[i].func(argc, argv);
                                                goto ddt_command_found;
                                        }
                                }
                                gs_ddt_printf(ddt, "[gs_ddt]: unrecognized command '%s'\n", argv[0]);
                        ddt_command_found:
                                ddt->cb[0] = '\0';
                        }
                } else if (ctx->key_pressed & GS_GUI_KEY_BACKSPACE && len > 0) {
                        // skip utf-8 continuation bytes
                        while ((ddt->cb[--len] & 0xc0) == 0x80 && len > 0);
                        ddt->cb[len] = '\0';
                } else if (n > 0) {
                        memcpy(ddt->cb + len, ctx->input_text, n);
                        len += n;
                        ddt->cb[len] = '\0';
                }

                // blinking cursor
                gs_gui_get_layout(ctx)->body.x -= 5;
                if ((int)(gs_platform_elapsed_time() / 666.0f) & 1)
                        gs_gui_text(ctx, "|");

                gs_gui_window_end(ctx);

                ddt->last_open_state = ddt->open;
        }
}
#endif // GS_DDT_IMPL

#endif // GS_DDT_H_
