#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <raylib.h>

#define FONT_NAME "ComicShannsMonoNerdFontMono-Bold.ttf"
#define FONT_SIZE 14
#define FONT_SPACING 2
#define TITLE_FONT_SCALE 1.25
#define LINE_MAX_LEN 128
#define DEFAULT_DRAW_OFFSET_X 5
#define DEFAULT_DRAW_OFFSET_Y 30 
#define CONNECTOR_OUTLINE_GAP 10
#define CONNECTOR_SPACING_X 200
#define CONNECTOR_SPACING_Y 30
#define FILE_LINE_MAX_LEN 256
#define DEFAULT_HIGHLIGHT_COLOR "DARKGOLD"
#define DEFAULT_FOREGROUND_COLOR_LIGHT "BLACK"
#define DEFAULT_BACKGROUND_COLOR_LIGHT "RAYWHITE"
#define DEFAULT_FOREGROUND_COLOR_DARK "LIGHTGRAY"
#define DEFAULT_BACKGROUND_COLOR_DARK "BLACK"
#define DEFAULT_WIRE_THICKNESS 2.0
#define DEFAULT_WIRE_STRAIGHT_FRACTION 0.2
#define ZOOM_RATE 0.02
#define MINIMUM_ZOOM 0.3
#define MAXIMUM_ZOOM 3.0

typedef struct pin {
    int number;
    char *name;
    int is_highlighted;
    int is_under_pointer;
} pin_t;

typedef struct wire_description {
    int c1;
    int c1_pin;
    int c2;
    int c2_pin;
    char *colour;
    float thickness;
    float straight_fraction;
    char *gauge;
    char *length;
    int is_highlighted;
} wire_description_t;

typedef struct connector_description {
    char *name;
    int number;
    char *type;
    char *mate;
    pin_t *pins;
    int n_pins;
    int mirror_lr;
} connector_description_t;

typedef struct harness_description {
    char *name;
    // TODO allow pin overrides of the wire length
    // Specified as a string to allow arbitrary units
    char *default_wire_length;
    // TODO allow pin overrides of the wire gauge
    char *default_wire_gauge;
    char *default_wire_colour;
    int n_connector_descriptions;
    connector_description_t *connector_descriptions;
    int n_wire_descriptions;
    wire_description_t *wire_descriptions;
    int changed;
} harness_description_t;

typedef struct connector {
    connector_description_t *description;
    Rectangle outline;
    char max_str[LINE_MAX_LEN];
    int max_letters;
    char max_pin_str[LINE_MAX_LEN];
    int max_pin_letters;
    char typerow[LINE_MAX_LEN];
    char pin_format[64];
    Font font;
    int font_spacing;
    int line_height;
    int is_highlighted;
} connector_t;

typedef struct harness {
    harness_description_t *description;
    int n_connectors;
    connector_t *connectors;
    Font title_font;
    int title_font_spacing;
} harness_t;

typedef struct program_state {
    const char *harness_filename;
    harness_description_t *harness_descriptions;
    harness_t *harnesses;
    int n_harnesses;
    int harness_index;
    Font title_font;
    Font connector_font;
    Vector2 mouse_position;
    Vector2 draw_offset;
    int dark_background;
    Color foreground_color;
    Color background_color;
    float zoom_level;
    int n_pins_under_pointer;
    connector_description_t *c1_under_pointer;
    pin_t *p1_under_pointer;
    connector_description_t *c2_under_pointer;
    pin_t *p2_under_pointer;
    Vector2 wire_drawing_first_end;
    Vector2 wire_drawing_second_end;

} program_state_t;

void create_connector(program_state_t *state, connector_t *c);
void free_connector(connector_t *c);
int draw_connector(program_state_t *state, connector_t *c, Vector2 position, int hidden);
void draw_harness(program_state_t *state);
float text_width(const char *str, Font font, int font_spacing);
int update_max_string(char *max_str, const char *str);
void parse_harness_description(program_state_t *state);
int parse_harness_properties(char *harness_properties, harness_description_t *h);
void parse_connector_header(char *header, connector_description_t *c);
int parse_pin_entry(char *pin_entry, connector_description_t *c);
int parse_wire_entry(char *wiring, harness_description_t *h);
void free_connector_description(connector_description_t *t);
void remove_newline(char *str);
int create_harnesses(program_state_t *state);
void free_harnesses(program_state_t *state);
Color get_color_from_string(const char *str);
int load_fonts(program_state_t *state);
int draw_text(program_state_t *state, Font font, char *text, Vector2 position, int font_spacing, Color default_color, Color highlighed_color, int force_highlight, int hidden, int *is_under_pointer);
char *read_line(char *ln, size_t len, FILE *fp);
void adjust_zoom(program_state_t *state, float amount);
int generate_boilerplate_harness_description(const char *filename, harness_description_t *h);
int export_harness_description(program_state_t *state);
harness_description_t *make_harness_description_template(void);
connector_description_t *add_connector_description(harness_description_t *h, const char *name, const char *type, const char *mate, int n_pins);
void free_harness_descriptions(program_state_t *state);
int export_template(int dark_background);
void try_to_delete_wire(program_state_t *state);
void try_to_add_wire(program_state_t *state);
void draw_pin_to_pointer(program_state_t *state);
void reset_pin_under_pointer_states(program_state_t *state);

int main(int argc, char **argv)
{
    int n_opts = 0;
    for (int i = 0; i < argc; ++i) {
        if (strcmp("--make-template", argv[i]) == 0) {
            int export_status = export_template(0);
            if (export_status != 0) {
                fprintf(stderr, "Error exporting harness description template.\n");
            }
            fprintf(stdout, "Exported harness description template.\n");
            return export_status;
        } else if (strcmp("--help", argv[i]) == 0) {
            fprintf(stdout, "USAGE: read the source code, try to remember, guess, or disassemble\n");
            return 0;
        } else if (strncmp("--", argv[i], 2) == 0) {
            fprintf(stderr, "What does '%s' mean?\n", argv[i]);
            return EXIT_FAILURE;
        }

    }
    if (argc != 2) {
        fprintf(stderr, "USAGE: read the source code, try to remember, guess, or disassemble\n");
        return EXIT_FAILURE;
    }

    program_state_t state = {0};
    state.zoom_level = 1.0;
    state.harness_filename = argv[1];
    parse_harness_description(&state);

    state.foreground_color = get_color_from_string(DEFAULT_FOREGROUND_COLOR_LIGHT);
    state.background_color = get_color_from_string(DEFAULT_BACKGROUND_COLOR_LIGHT);
    if (state.dark_background == 1) {
        state.foreground_color = get_color_from_string(DEFAULT_FOREGROUND_COLOR_DARK);
        state.background_color = get_color_from_string(DEFAULT_BACKGROUND_COLOR_DARK);
    }

    int running = 1;

    SetTraceLogLevel(LOG_NONE);
    int monitor = GetCurrentMonitor();
    int screen_width = GetMonitorWidth(monitor);
    int screen_height = GetMonitorHeight(monitor);
    InitWindow(screen_width, screen_height, "Harness");
    SetTargetFPS(60);
    SetGesturesEnabled(GESTURE_PINCH_IN | GESTURE_PINCH_OUT);

    int font_status = load_fonts(&state);
    if (font_status != 0) {
        fprintf(stderr, "Unable to load font '%s'\n", FONT_NAME);
        return EXIT_FAILURE;
    }

    state.draw_offset = (Vector2){DEFAULT_DRAW_OFFSET_X, DEFAULT_DRAW_OFFSET_Y};

    int key = 0;
    GetMouseDelta();

    int export_status = 0;
    int harness_status = 0;

    while (running) {
        state.mouse_position = GetMousePosition();
        (void)create_harnesses(&state);
        BeginDrawing();
            ClearBackground(state.background_color);
            draw_harness(&state);
        EndDrawing();

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && state.n_pins_under_pointer > 0) {
            draw_pin_to_pointer(&state);
        } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && state.n_pins_under_pointer == 2) {
            try_to_add_wire(&state);
            reset_pin_under_pointer_states(&state);
        } 
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
            Vector2 mouse_movement = GetMouseDelta();
            state.draw_offset.x += mouse_movement.x;
            state.draw_offset.y += mouse_movement.y;
        }

        if (IsKeyDown(KEY_MINUS))
        {
            adjust_zoom(&state, +ZOOM_RATE);
            load_fonts(&state);
        } else if (IsKeyDown(KEY_EQUAL) && (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))) {
            adjust_zoom(&state, -ZOOM_RATE);
            load_fonts(&state);
        } else if (IsKeyDown(KEY_ZERO)) {
            state.zoom_level = 1.0;
            load_fonts(&state);
        }
        if (IsKeyDown(KEY_L)) {
            state.draw_offset.x -= 10;
        } else if (IsKeyDown(KEY_H)) {
            state.draw_offset.x += 10;
        }
        if (IsKeyDown(KEY_J)) {
            state.draw_offset.y -= 10;
        } else if (IsKeyDown(KEY_K)) {
            state.draw_offset.y += 10;
        }

        while ((key = GetKeyPressed())) {
            switch (key) {
                case KEY_Q:
                    running = 0;
                    break;
                case KEY_BACKSPACE:
                    state.draw_offset = (Vector2){DEFAULT_DRAW_OFFSET_X, DEFAULT_DRAW_OFFSET_Y};
                    break;
                case KEY_S:
                    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
                        export_status = export_harness_description(&state);
                        if (export_status != 0) {
                            fprintf(stderr, "Error saving harness description.\n");
                        }
                    }
                    break;
                case KEY_N:
                    if (IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
                        export_status = export_template(state.dark_background);
                        if (export_status != 0) {
                            fprintf(stderr, "Error exporting harness description template.\n");
                        }
                    } else {
                        state.harness_index++;
                        if (state.harness_index >= state.n_harnesses) {
                            state.harness_index = state.n_harnesses - 1;
                        }
                    }
                    break;
                case KEY_P:
                        state.harness_index--;
                        if (state.harness_index < 0) {
                            state.harness_index = 0;
                        }
                    break;
                case KEY_D:
                    try_to_delete_wire(&state);
                    break;
                default:
                    break;
            }
        }
        free_harnesses(&state);
    }

    free_harness_descriptions(&state);

    return 0;
}

void create_connector(program_state_t *state, connector_t *c)
{
    if (c == NULL) {
        fprintf(stderr, "Invalid NULL pointer in call to create_connector\n");
        return;
    }

    connector_description_t *cd = c->description;
    c->font = state->connector_font;
    c->line_height = c->font.baseSize + 2;
    c->font_spacing = FONT_SPACING;
    c->max_letters = update_max_string(c->max_str, cd->name);
    snprintf(c->typerow, LINE_MAX_LEN, "%s %s (%d pins)", cd->type, cd->mate, cd->n_pins);
    c->max_letters = update_max_string(c->max_str, c->typerow);
    for (int i = 0; i < cd->n_pins; ++i) {
        c->max_pin_letters = update_max_string(c->max_pin_str, cd->pins[i].name);
    }
    c->max_letters = update_max_string(c->max_str, c->max_pin_str);
    c->outline.width = text_width(c->max_str, c->font, c->font_spacing) + CONNECTOR_OUTLINE_GAP * 2;
    c->outline.height = c->line_height * (2 + cd->n_pins) + CONNECTOR_OUTLINE_GAP * 2;

    return;
}

void free_connector(connector_t *c) {
    free(c);
    c = NULL;
}

int draw_connector(program_state_t *state, connector_t *c, Vector2 position, int hidden)
{
    c->outline.x = position.x;
    c->outline.y = position.y;

    int highlighting_updated = 0;

    connector_description_t *cd = c->description;
    wire_description_t *wd = NULL;
    pin_t *p = NULL;
    harness_description_t *hd = &state->harness_descriptions[state->harness_index];
    int n_wires = hd->n_wire_descriptions;

    int yoff = c->outline.y + CONNECTOR_OUTLINE_GAP;
    int xoff = c->outline.x + c->outline.width / 2 - text_width(cd->name, c->font, c->font_spacing) / 2;
    int xoff1 = 0;
    int yoff1 = 0;
    Color highlighted_color = get_color_from_string(DEFAULT_HIGHLIGHT_COLOR);
    float line_thickness = 1.5;
    c->is_highlighted = CheckCollisionPointRec(state->mouse_position, c->outline);
    if (c->is_highlighted) {
        line_thickness += 1.0;
    }
    if (!hidden) {
        DrawRectangleRoundedLinesEx(c->outline, 0.1, 1, line_thickness * state->connector_font.baseSize / FONT_SIZE, state->foreground_color);
    }
    draw_text(state, c->font, cd->name, (Vector2){xoff, yoff}, c->font_spacing, state->foreground_color, state->foreground_color, 0, hidden, NULL);


    int old_highlighting = 0;
    yoff += c->line_height;
    xoff = c->outline.x + c->outline.width / 2 - text_width(c->typerow, c->font, c->font_spacing) / 2;
    if (!hidden) {
        DrawTextEx(c->font, c->typerow, (Vector2){xoff, yoff}, c->font.baseSize, FONT_SPACING, state->foreground_color);
    }

    char pin_line[LINE_MAX_LEN] = {0};
    if (cd->mirror_lr) {
        snprintf(c->pin_format, 64, "%%3d %%s");
        snprintf(pin_line, LINE_MAX_LEN, c->pin_format, cd->pins[0].number, cd->pins[0].name);
        xoff = c->outline.x + CONNECTOR_OUTLINE_GAP;
    } else {
        snprintf(c->pin_format, 64, "%%%ds %%3d", c->max_pin_letters);
        snprintf(pin_line, LINE_MAX_LEN, c->pin_format, cd->pins[0].name, cd->pins[0].number);
        xoff = c->outline.x + c->outline.width - CONNECTOR_OUTLINE_GAP - text_width(pin_line, c->font, c->font_spacing);
    }
    for (int i = 0; i < cd->n_pins; ++i) {
        p = &cd->pins[i];
        yoff += c->line_height;
        if (cd->mirror_lr) {
            snprintf(pin_line, LINE_MAX_LEN, c->pin_format, p->number, p->name);
        } else {
            snprintf(pin_line, LINE_MAX_LEN, c->pin_format, p->name, p->number);
        }
        // Check if pin's wire is highlighted
        old_highlighting = p->is_highlighted;
        for (int k = 0; k < n_wires; ++k) {
            wd = &hd->wire_descriptions[k];
            if (!p->is_highlighted && ((wd->c1 == cd->number && wd->c1_pin == p->number) || (wd->c2 == cd->number && wd->c2_pin == p->number))) {
                if (wd->is_highlighted) {
                    p->is_highlighted = 1;
                }
            }
        }
        Vector2 pin_line_size = MeasureTextEx(c->font, pin_line, c->font.baseSize, c->font_spacing);
        p->is_highlighted = draw_text(state, c->font, pin_line, (Vector2){xoff, yoff}, c->font_spacing, state->foreground_color, highlighted_color, p->is_highlighted, hidden, &p->is_under_pointer);
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && p->is_under_pointer) {
            xoff1 = xoff;
            if (!cd->mirror_lr)  {
                xoff1 += pin_line_size.x;
            }
            yoff1 = yoff + pin_line_size.y / 2.0;
            if (state->n_pins_under_pointer == 0) {
                state->wire_drawing_first_end = (Vector2){xoff1, yoff1};
                state->c1_under_pointer = cd;
                state->p1_under_pointer = p;
                state->n_pins_under_pointer = 1;
            } else if (state->n_pins_under_pointer == 1 && p != state->p1_under_pointer) {
                state->wire_drawing_second_end = (Vector2){xoff1, yoff1};
                state->c2_under_pointer = cd;
                state->p2_under_pointer = p;
                state->n_pins_under_pointer = 2;
            } else if (state->n_pins_under_pointer == 2 && p != state->p2_under_pointer) {
                state->wire_drawing_second_end = (Vector2){xoff1, yoff1};
                state->c2_under_pointer = cd;
                state->p2_under_pointer = p;
            }
        }
        if (p->is_highlighted != old_highlighting) {
            highlighting_updated = 1;
        }
        // highlight its wire and the target connector's pin 
        if (p->is_highlighted) {
            for (int k = 0; k < n_wires; ++k) {
                wd = &hd->wire_descriptions[k];
                if (!wd->is_highlighted && ((wd->c1 == cd->number && wd->c1_pin == p->number) || (wd->c2 == cd->number && wd->c2_pin == p->number))) {
                    wd->is_highlighted = 1;
                    highlighting_updated = 1;
                }
            }
        }
    }

    return highlighting_updated;
}

void draw_harness(program_state_t *state)
{
    // TODO draw harness name with (harness_index/n_harnesses) indicating which harness this is.
    
    if (state->harness_index < 0 || state->harness_index >= state->n_harnesses) {
        return;
    }

    harness_t *h = &state->harnesses[state->harness_index];
    if (h->n_connectors == 0) {
        return;
    }

    // Draw all non-mirrored connectors at the same x position, same for mirrored connectors
    Vector2 connector_pos_left = {0};
    Vector2 connector_pos_right = {0};
    int max_width = 0;
    connector_t *c = NULL;
    // Prepare harnesses to capture pin highlighting by mouse
    int highlighting_updated = 0;
    int highlighting_completed = 0;
    int iter = 0;
    while (highlighting_completed == 0) {
        highlighting_updated = 0;
        connector_pos_left = state->draw_offset;
        for (int i = 0; i < h->n_connectors; ++i) {
            c = &h->connectors[i];
            if (!c->description->mirror_lr) {
                highlighting_updated += draw_connector(state, c, connector_pos_left, 1);
                if (c->outline.width > max_width) {
                    max_width = c->outline.width;
                }
                connector_pos_left.y += c->outline.height + CONNECTOR_SPACING_Y;
            }
        }
        connector_pos_right = (Vector2){connector_pos_left.x + max_width + (float)CONNECTOR_SPACING_X * state->zoom_level, state->draw_offset.y};
        for (int i = 0; i < h->n_connectors; ++i) {
            c = &h->connectors[i];
            if (c->description->mirror_lr) {
                highlighting_updated += draw_connector(state, c, connector_pos_right, 1);
                connector_pos_right.y += c->outline.height + CONNECTOR_SPACING_Y;
            }
        }
        // Prepare harness right side to capture highlighting
        if (highlighting_updated == 0) {
            highlighting_completed = 1;
        }
    }
    // Draw harness left side
    connector_pos_left = state->draw_offset;
    for (int i = 0; i < h->n_connectors; ++i) {
        c = &h->connectors[i];
        if (!c->description->mirror_lr) {
            draw_connector(state, c, connector_pos_left, 0);
            connector_pos_left.y += c->outline.height + CONNECTOR_SPACING_Y;
        }
    }
    // Draw harness right side
    connector_pos_right = (Vector2){connector_pos_left.x + max_width + (float)CONNECTOR_SPACING_X * state->zoom_level, state->draw_offset.y};
    for (int i = 0; i < h->n_connectors; ++i) {
        c = &h->connectors[i];
        if (c->description->mirror_lr) {
            draw_connector(state, c, connector_pos_right, 0);
            connector_pos_right.y += c->outline.height + CONNECTOR_SPACING_Y;
        }
    }

    float dx = 0.0;
    int i_left = 0;
    int i_right = 0;
    float y_left = 0.0;
    float y_right = 0.0;

    harness_description_t *hd = h->description;
    connector_t *cleft = NULL;
    connector_t *cright = NULL;
    wire_description_t *wd = NULL;
    int cleft_index = 0;
    int cright_index = 0;
    int pleft_index = 0;
    int pright_index = 0;
    for (int i = 0; i < hd->n_wire_descriptions; ++i) {
        wd = &hd->wire_descriptions[i];
        cleft_index = wd->c1 - 1;
        if (cleft_index < 0 || cleft_index >= h->n_connectors) {
            fprintf(stderr, "Invalid source connector number for wire %d\n", i + 1);
            continue;
        }
        cright_index = wd->c2 - 1;
        if (cright_index < 0 || cright_index >= h->n_connectors) {
            fprintf(stderr, "Invalid target connector number for wire %d\n", i + 1);
            continue;
        }
        cleft = &h->connectors[cleft_index];
        cright = &h->connectors[cright_index];

        float y0_left = cleft->outline.y + CONNECTOR_OUTLINE_GAP + cleft->font.baseSize * 1.5;
        float y0_right = cright->outline.y + CONNECTOR_OUTLINE_GAP + cright->font.baseSize * 1.5;

        float x_left = cleft->outline.x;
        if (!cleft->description->mirror_lr) {
            x_left += cleft->outline.width;
        }
        float x_right = cright->outline.x;
        if (!cright->description->mirror_lr) {
            x_right += cright->outline.width;
        }
        i_left = wd->c1_pin;
        i_right = wd->c2_pin;
        y_left = y0_left + (float)(i_left * cleft->line_height);
        y_right = y0_right + (float)(i_right * cright->line_height);
        dx = wd->straight_fraction * (float)CONNECTOR_SPACING_X * state->zoom_level;
        Vector2 points[] = {{x_left, y_left}, {x_left + dx, y_left}, {x_right - dx, y_right}, {x_right, y_right}};
        if (x_right == x_left) {
            // Pins are on the same end of the harness
            if (cleft->description->mirror_lr) {
                points[1].x -= 2 * dx;
            } else {
                points[2].x += 2 * dx;
            }
        }
        Color outline_wire_color = state->foreground_color;
        float outline_wire_thickness = wd->thickness + 0.5;
        // Draw wire
        if (wd->is_highlighted) {
            outline_wire_color = get_color_from_string(DEFAULT_HIGHLIGHT_COLOR);
            outline_wire_thickness = wd->thickness + 4;
        }
        DrawSplineBezierCubic(points, 4, outline_wire_thickness * state->zoom_level, outline_wire_color);
        DrawSplineBezierCubic(points, 4, wd->thickness * state->zoom_level, get_color_from_string(wd->colour));
    }

    Vector2 title_size = MeasureTextEx(h->title_font, h->description->name, h->title_font.baseSize, h->title_font_spacing);
    DrawTextEx(h->title_font, h->description->name, (Vector2){state->draw_offset.x, state->draw_offset.y - title_size.y - 5}, h->title_font.baseSize, h->title_font_spacing, BLACK); 

}

float text_width(const char *str, Font font, int font_spacing)
{
    return MeasureTextEx(font, str, font.baseSize, font_spacing).x;
}

int update_max_string(char *max_str, const char *str)
{
    if (strlen(str) > strlen(max_str)) {
        snprintf(max_str, LINE_MAX_LEN, "%s", str);
    }

    return (int)strlen(max_str);
}

void parse_harness_description(program_state_t *state)
{
    FILE *fp = fopen(state->harness_filename, "r");
    if (fp == NULL) {
        fprintf(stderr, "Unable to open %s\n", state->harness_filename);
        return;
    }

    void *mem = NULL;
    int status = 0;
    size_t len = 0;
    char ln[FILE_LINE_MAX_LEN] = {0};
    char *fgetsptr = NULL;
    harness_description_t *h = NULL;

    connector_description_t *cd = NULL;
    wire_description_t *wd = NULL;

    // ignores comments
    while ((read_line(ln, FILE_LINE_MAX_LEN, fp)) != NULL) {
        if (strcmp("dark_background", ln) == 0) {
            state->dark_background = 1;
        } else if (strncmp("harness", ln, 7) == 0) {
            fgetsptr = read_line(ln, FILE_LINE_MAX_LEN, fp);
            mem = realloc(state->harness_descriptions, sizeof *state->harness_descriptions * (state->n_harnesses + 1));
            if (mem == NULL) {
                fprintf(stderr, "Error allocating memory\n");
                return;
            }
            state->harness_descriptions = mem;
            state->n_harnesses++;
            h = &state->harness_descriptions[state->n_harnesses - 1];
            memset(h, 0, sizeof *h);
            h->name = strdup(ln);
            fgetsptr = read_line(ln, FILE_LINE_MAX_LEN, fp);
            if (fgetsptr != NULL) {
                status = parse_harness_properties(ln, h);
            }
        } else if (strncmp("connector", ln, 9) == 0) {
            mem = realloc(h->connector_descriptions, sizeof *h->connector_descriptions * (h->n_connector_descriptions + 1));
            if (mem == NULL) {
                fclose(fp);
                fprintf(stderr, "Error allocating memory\n");
                return;
            }
            h->connector_descriptions = mem;
            h->n_connector_descriptions++;
            cd = &h->connector_descriptions[h->n_connector_descriptions - 1];
            memset(cd, 0, sizeof *cd);
            cd->number = h->n_connector_descriptions;
            fgetsptr = read_line(ln, FILE_LINE_MAX_LEN, fp);
            if (fgetsptr != NULL) {
                parse_connector_header(ln, cd);
            }
            while ((read_line(ln, FILE_LINE_MAX_LEN, fp)) != NULL) {
                if (ln[0] == '.') {
                    // End of pin list for this connector
                    break;
                }
                status = parse_pin_entry(ln, cd);
                if (status != 0) {
                    break;
                }
            }
        } else if (strncmp("wiring", ln, 6) == 0) {
            while ((read_line(ln, FILE_LINE_MAX_LEN, fp)) != NULL) {
                if (ln[0] == '.') {
                    // End of wiring table for this harness
                    break;
                }
                status = parse_wire_entry(ln, h);
                if (status != 0) {
                    break;
                }
            }
        }
    }

    fclose(fp);

    return;

}

int parse_harness_properties(char *harness_properties, harness_description_t *h)
{
    char *token, *string, *tofree;
    tofree = string = strdup(harness_properties);

    void *mem = NULL;
    token = strsep(&string, ",");
    if (token == NULL) {
        fprintf(stderr, "%s: expected <default_wire_length>,<default_wire_gauge>,<default_wire_colour>\n", h->name);
        return 1;
    }
    h->default_wire_length = strdup(token);
    token = strsep(&string, ",");
    if (token == NULL || strlen(token) == 0) {
        fprintf(stderr, "%s: missing <default_wire_gauge>\n", h->name);
        return 1;
    }
    h->default_wire_gauge = strdup(token);
    token = strsep(&string, ",");
    if (token == NULL || strlen(token) == 0) {
        fprintf(stderr, "%s: missing <default_wire_colour>\n", h->name);
        return 1;
    }
    h->default_wire_colour = strdup(token);

    free(tofree);

    return 0;
}

void parse_connector_header(char *header, connector_description_t *c)
{
    char *token, *string, *tofree;
    tofree = string = strdup(header);

    token = strsep(&string, ",");
    if (token != NULL) {
        c->name = strdup(token);
    }
    token = strsep(&string, ",");
    if (token != NULL) {
        c->type = strdup(token);
    }
    token = strsep(&string, ",");
    if (token != NULL) {
        c->mate = strdup(token);
    }
    c->mirror_lr = 0;
    token = strsep(&string, ",");
    if (token != NULL && strcmp("reversed", token) == 0) {
        c->mirror_lr = 1;
    }

    free(tofree);
}

int parse_pin_entry(char *pin_entry, connector_description_t *c)
{
    char *token, *string, *tofree;
    tofree = string = strdup(pin_entry);

    void *mem = NULL;
    pin_t *p = NULL;
    token = strsep(&string, " ");
    if (token == NULL) {
        fprintf(stderr, "%s invalid entry for pin %d\n", c->name, c->n_pins + 1);
        return 1;
    }

    mem = realloc(c->pins, sizeof *c->pins * (c->n_pins + 1));
    if (mem == NULL) {
        fprintf(stderr, "Error allocating memory\n");
        return 2;
    }
    c->pins = mem;
    c->n_pins++;
    p = &c->pins[c->n_pins-1];
    memset(p, 0, sizeof *p);
    p->number = atoi(token);

    if (string == NULL || strlen(string) == 0) {
        fprintf(stderr, "%s missing name for pin %d\n", c->name, p->number);
        return 1;
    }
    p->name = strdup(string);

    free(tofree);

    return 0;
}

int parse_wire_entry(char *wiring, harness_description_t *h)
{
    char *token, *string, *tofree;
    tofree = string = strdup(wiring);

    void *mem = NULL;
    wire_description_t *w = NULL;
    token = strsep(&string, ",");
    if (token == NULL) {
        fprintf(stderr, "%s: invalid wire entry %s\n", h->name, wiring);
        return 1;
    }

    mem = realloc(h->wire_descriptions, sizeof *h->wire_descriptions * (h->n_wire_descriptions + 1));
    if (mem == NULL) {
        fprintf(stderr, "Error allocating memory\n");
        return 2;
    }
    h->wire_descriptions = mem;
    h->n_wire_descriptions++;
    w = &h->wire_descriptions[h->n_wire_descriptions - 1];
    memset(w, 0, sizeof *w);

    w->colour = h->default_wire_colour;
    w->thickness = DEFAULT_WIRE_THICKNESS;
    w->straight_fraction = DEFAULT_WIRE_STRAIGHT_FRACTION;
    w->length = h->default_wire_length;
    w->gauge = h->default_wire_gauge;

    w->c1 = atoi(token);
    token = strsep(&string, ",");
    if (token == NULL) {
        fprintf(stderr, "%s: invalid wire entry %s\n", h->name, wiring);
        return 1;
    }
    w->c1_pin = atoi(token);

    token = strsep(&string, ",");
    if (token == NULL) {
        fprintf(stderr, "%s: invalid wire entry %s\n", h->name, wiring);
        return 1;
    }
    w->c2 = atoi(token);

    token = strsep(&string, ",");
    if (token == NULL) {
        fprintf(stderr, "%s: invalid wire entry %s\n", h->name, wiring);
        return 1;
    }
    w->c2_pin = atoi(token);

    token = strsep(&string, ",");
    if (token != NULL) {
        w->colour = strdup(token);
    }

    token = strsep(&string, ",");
    if (token != NULL) {
        w->thickness = atof(token);
        if (w->thickness < 0 || w->thickness > 20) {
            fprintf(stderr, "Invalid wire thickness %f\n", w->thickness);
            w->thickness = DEFAULT_WIRE_THICKNESS;
        }
    }

    free(tofree);

    return 0;
}

void free_connector_description(connector_description_t *t)
{
    free(t->name);
    free(t->type);
    free(t->mate);
    pin_t *p = NULL;
    for (int i = 0; i < t->n_pins; ++i) {
        p = &t->pins[i];
        free(p->name);
    }
}

void remove_newline(char *str)
{
    if (str == NULL)
    {
        return;
    }

    // remove newline if present
    size_t len = strlen(str);
    if (str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }

}

int create_harnesses(program_state_t *state)
{
    if (state == NULL || state->n_harnesses == 0) {
        return 1;
    }

    state->harnesses = malloc(sizeof *state->harnesses * state->n_harnesses);
    if (state->harnesses == NULL) {
        fprintf(stderr, "Error allocating memory\n");
        return 2;
    }
    memset(state->harnesses, 0, sizeof *state->harnesses * state->n_harnesses);
    harness_t *h = NULL;
    for (int i = 0; i < state->n_harnesses; ++i) {
        h = &state->harnesses[i];
        h->title_font_spacing = FONT_SPACING;
        h->title_font = state->title_font;
        h->description = &state->harness_descriptions[i];
        h->n_connectors = h->description->n_connector_descriptions;
        h->connectors = malloc(sizeof *h->connectors * h->n_connectors);
        if (h->connectors == NULL) {
            fprintf(stderr, "Error allocating memory\n");
            return 3;
        }
        memset(h->connectors, 0, sizeof *h->connectors * h->n_connectors);
        connector_t *c = NULL;
        for (int i = 0; i < h->n_connectors; ++i) {
            c = &h->connectors[i];
            c->description = &h->description->connector_descriptions[i];
            // reset pin highlighting
            for (int k = 0; k < c->description->n_pins; ++k) {
                c->description->pins[k].is_highlighted = 0;
            }
            create_connector(state, c);
        }

        // reset wire highlighting
        for (int i = 0; i < h->description->n_wire_descriptions; ++i) {
            h->description->wire_descriptions[i].is_highlighted = 0;
        }

    }

    return 0;
}

void free_harnesses(program_state_t *state)
{
    harness_t *h = NULL;
    for (int i = 0; i < state->n_harnesses; ++i) {
        h = &state->harnesses[i];
        free(h->connectors);
    }
    free(state->harnesses);

}

Color get_color_from_string(const char *str)
{
    if (str == NULL) {
        return RED;
    }

    // Raylib colours
    Color c = BLACK;
    if (strcmp("LIGHTGRAY", str) == 0) {
        c = LIGHTGRAY;
    } else if (strcmp("GRAY", str) == 0) {
        c = GRAY;
    } else if (strcmp("DARKGRAY", str) == 0) {
        c = DARKGRAY;
    } else if (strcmp("YELLOW", str) == 0) {
        c = YELLOW;
    } else if (strcmp("GOLD", str) == 0) {
        c = GOLD;
    } else if (strcmp("DARKGOLD", str) == 0) {
        c = (Color){ 230, 183, 0, 255 }; 
    } else if (strcmp("DARKERGOLD", str) == 0) {
        c = (Color){ 204, 162, 0, 255 }; 
    } else if (strcmp("ORANGE", str) == 0) {
        c = ORANGE;
    } else if (strcmp("PINK", str) == 0) {
        c = PINK;
    } else if (strcmp("RED", str) == 0) {
        c = RED;
    } else if (strcmp("MAROON", str) == 0) {
        c = MAROON;
    } else if (strcmp("GREEN", str) == 0) {
        c = GREEN;
    } else if (strcmp("LIME", str) == 0) {
        c = LIME;
    } else if (strcmp("DARKGREEN" , str) == 0) {
        c = DARKGREEN;
    } else if (strcmp("SKYBLUE", str) == 0) {
        c = SKYBLUE;
    } else if (strcmp("BLUE", str) == 0) {
        c = BLUE;
    } else if (strcmp("DARKBLUE", str) == 0) {
        c = DARKBLUE;
    } else if (strcmp("PURPLE", str) == 0) {
        c = PURPLE;
    } else if (strcmp("VIOLET", str) == 0) {
        c = VIOLET;
    } else if (strcmp("DARKPURPLE", str) == 0) {
        c = DARKPURPLE;
    } else if (strcmp("BEIGE", str) == 0) {
        c = BEIGE;
    } else if (strcmp("BROWN", str) == 0) {
        c = BROWN;
    } else if (strcmp("DARKBROWN" , str) == 0) {
        c = DARKBROWN;
    } else if (strcmp("WHITE", str) == 0) {
        c = WHITE;
    } else if (strcmp("BLANK", str) == 0) {
        c = BLANK;
    } else if (strcmp("MAGENTA", str) == 0) {
        c = MAGENTA;
    } else if (strcmp("RAYWHITE", str) == 0) {
        c = RAYWHITE;
    } 

    return c;
}

int load_fonts(program_state_t *state)
{
    int font_size = FONT_SIZE * state->zoom_level;
    if (font_size < 1) {
        font_size = 1;
    }
    if (font_size > 50) {
        font_size = 50;
    }
    if (IsFontValid(state->connector_font)) {
        UnloadFont(state->connector_font);
    }
    char font_filename[FILENAME_MAX] = {0};
    char *home = getenv("HOME");
    if (home == NULL) {
        fprintf(stderr, "Unable to get HOME environment variable\n");
        return 1;
    }
    snprintf(font_filename, FILENAME_MAX, "%s/bin/%s", home, FONT_NAME);
    state->connector_font = LoadFontEx(font_filename, font_size, NULL, 0);
    if (!IsFontValid(state->connector_font)) {
        return 1;
    }
    if (IsFontValid(state->title_font)) {
        UnloadFont(state->title_font);
    }
    state->title_font = LoadFontEx(font_filename, (int)(TITLE_FONT_SCALE * font_size), NULL, 0);
    if (!IsFontValid(state->title_font)) {
        return 1;
    }

    return 0;
}

int draw_text(program_state_t *state, Font font, char *text, Vector2 position, int font_spacing, Color default_color, Color highlighed_color, int force_highlight, int hidden, int *is_under_pointer)
{
    Vector2 text_size = MeasureTextEx(font, text, font.baseSize, font_spacing);
    Rectangle text_box = (Rectangle){position.x, position.y, text_size.x, text_size.y};
    Color color = default_color;
    int highlighted = CheckCollisionPointRec(state->mouse_position, text_box);
    if (is_under_pointer != NULL) {
        *is_under_pointer = highlighted;
    }
    if (force_highlight) {
        highlighted = 1;
    }
    if (highlighted) {
        color = highlighed_color;
    }
    if (!hidden) {
        DrawTextEx(font, text, position, font.baseSize, font_spacing, color);
    }

    return highlighted;
}

void reset_highlighting(program_state_t *state)
{

}

// Ignores comments and empty lines, removes trailing newline
char *read_line(char *ln, size_t len, FILE *fp)
{
    char *fgetsptr = NULL;
    while((fgetsptr = fgets(ln, FILE_LINE_MAX_LEN, fp)) != NULL) {
        if (ln[0] != '\n' && ln[0] != '#') {
            break;
        }
    }
    if (fgetsptr != NULL) {
        remove_newline(ln);
    }
    return fgetsptr;
}

void adjust_zoom(program_state_t *state, float amount)
{
    state->zoom_level -= amount; 
    if (state->zoom_level < MINIMUM_ZOOM) {
        state->zoom_level = MINIMUM_ZOOM;
    } else if (state->zoom_level > MAXIMUM_ZOOM) {
        state->zoom_level = MAXIMUM_ZOOM;
    }

}

int export_harness_description(program_state_t *state)
{
    FILE *fp = fopen(state->harness_filename, "w");
    if (fp == NULL) {
        fprintf(stderr, "Error opening %s for writing\n", state->harness_filename);
        return 1;
    }

    harness_description_t *h = NULL;
    connector_description_t *c = NULL;
    pin_t *p = NULL;
    wire_description_t *w = NULL;
    fprintf(fp, "%sdark_background\n", state->dark_background ? "" : "#");
    fprintf(fp, "# comments like this and empty lines are ignored\n");
    fprintf(fp, "# Format: consists of a 3-line 'harness' header\n");
    fprintf(fp, "# followed by one or more 'connector' descriptions\n");
    fprintf(fp, "# and a single 'wiring' section. \n");
    fprintf(fp, "\n");
    fprintf(fp, "# The end of an enumerated list, such as a pin list, is denoted by '.' on a\n");
    fprintf(fp, "# line by itself.\n");
    fprintf(fp, "# Pins must appear in increasing order.\n");
    for (int i = 0; i < state->n_harnesses; ++i) {
        h = &state->harness_descriptions[0];
        fprintf(fp, "\n");
        fprintf(fp, "harness %d\n", i + 1);
        fprintf(fp, "%s\n", h->name);
        fprintf(fp, "%s,%s,%s\n", h->default_wire_length, h->default_wire_gauge, h->default_wire_colour);
        fprintf(fp, "\n");
        for (int j = 0; j < h->n_connector_descriptions; ++j) {
            c = &h->connector_descriptions[j];
            fprintf(fp, "connector %d\n", j + 1);
            fprintf(fp, "# <name>,<type>,<mate>[,reversed]\n");
            fprintf(fp, "%s,%s,%s%s\n", c->name, c->type, c->mate, c->mirror_lr ? ",reversed" : "");
            for (int k = 0; k < c->n_pins; ++k) {
                p = &c->pins[k];
                fprintf(fp, "%d %s\n", p->number, p->name);
            }
            fprintf(fp, ".\n");
            fprintf(fp, "\n");
        }

        fprintf(fp, "wiring\n");
        fprintf(fp, "# <src_conn_#>,<src_pin_#>,<dst_conn_#>,<dst_pin_#>[,<wire_colour>][,<wire_thickness][,<wire_gauge>]\n");
        for (int j = 0; j < h->n_wire_descriptions; ++j) {
            w = &h->wire_descriptions[j];
            fprintf(fp, "%d,%d,%d,%d", w->c1, w->c1_pin, w->c2, w->c2_pin);
            if (strcmp(w->colour, h->default_wire_colour) != 0) {
                fprintf(fp, ",%s", w->colour);
                if (w->thickness != DEFAULT_WIRE_THICKNESS) {
                    fprintf(fp, ",%g", w->thickness);
                    if (strcmp(w->gauge, h->default_wire_gauge) != 0) {
                        fprintf(fp, ",%s", w->gauge);
                    }
                }
            }
            fprintf(fp, "\n");
        }
        fprintf(fp, ".\n");
    }

    fclose(fp);
    fflush(fp);

    return 0;
}

harness_description_t *make_harness_description_template(void)
{
    harness_description_t *h = malloc(sizeof *h);
    if (h == NULL) {
        fprintf(stderr, "Error allocating memory\n");
        return NULL;
    }
    memset(h, 0, sizeof *h);

    h->name = strdup("<name>");
    h->default_wire_length = strdup("30cm");
    h->default_wire_gauge = strdup("26awg");
    h->default_wire_colour = strdup("GRAY");

    connector_description_t *c = add_connector_description(h, "J1", "header", "plug", 5);
    if (c == NULL) {
        return NULL;
    }
    c->pins[0].name = strdup("+5V");
    c->pins[1].name = strdup("AGND");

    c = add_connector_description(h, "J2", "header", "plug", 2);
    if (c == NULL) {
        return NULL;
    }
    c->pins[0].name = strdup("3V3");
    c->pins[1].name = strdup("DGND");

    c = add_connector_description(h, "J3", "DB9", "socket", 5);
    if (c == NULL) {
        return NULL;
    }
    c->mirror_lr = 1;
    c->pins[0].name = strdup("+5V");
    c->pins[1].name = strdup("+5V_RTN");
    c->pins[2].name = strdup("NC");
    c->pins[3].name = strdup("3V3");
    c->pins[4].name = strdup("3V3_RTN");

    return h;

}

connector_description_t *add_connector_description(harness_description_t *h, const char *name, const char *type, const char *mate, int n_pins)
{
    void *mem = realloc(h->connector_descriptions, sizeof *h->connector_descriptions * (h->n_connector_descriptions + 1));
    if (mem == NULL) {
        fprintf(stderr, "Error allocating memory\n");
        return NULL;
    }
    h->connector_descriptions = mem;
    h->n_connector_descriptions++;

    connector_description_t *c = &h->connector_descriptions[h->n_connector_descriptions - 1];
    memset(c, 0, sizeof *c);
    c->name = strdup(name);
    c->number = h->n_connector_descriptions;
    c->type = strdup(type);
    c->mate = strdup(mate);
    c->n_pins = n_pins;
    c->pins = malloc(sizeof *c->pins * n_pins);
    if (c->pins == NULL) {
        return NULL;
    }
    for (int i = 0; i < n_pins; ++i) {
        c->pins[i].name = strdup("NC");
        c->pins[i].number = i + 1;
        c->pins[i].is_highlighted = 0;
    }

    return c;
}

void free_harness_descriptions(program_state_t *state)
{
    harness_description_t *hd = NULL;
    for (int i = 0; i < state->n_harnesses; ++i) {
        hd = &state->harness_descriptions[i];
        for (int j = 0; j < hd->n_connector_descriptions; ++j) {
            free_connector_description(&hd->connector_descriptions[j]);
        }
        free(hd->connector_descriptions);
        free(hd->wire_descriptions);
    }
    free(state->harness_descriptions);
    state->harness_descriptions = NULL;
    state->n_harnesses = 0;
}

int export_template(int dark_background) 
{
    program_state_t template_state = {0};
    template_state.dark_background = dark_background;

    template_state.harness_filename = "template_harness.txt";
    template_state.harness_descriptions = make_harness_description_template();
    template_state.n_harnesses = 1;
    int export_status = export_harness_description(&template_state);
    free_harness_descriptions(&template_state);

    return export_status;
}

void try_to_delete_wire(program_state_t *state)
{
    // Is a pin under the mouse pointer?

    connector_t *c = NULL;
    connector_description_t *cd = NULL;
    wire_description_t *wd = NULL;
    pin_t *p = NULL;
    int yoff = 0;
    int xoff = 0;

    harness_t *h = &state->harnesses[state->harness_index];
    harness_description_t *hd = &state->harness_descriptions[state->harness_index];
    int n_wires = hd->n_wire_descriptions;
    for (int j = 0; j < hd->n_connector_descriptions; ++j) {
        cd = &hd->connector_descriptions[j];
        for (int k = 0; k < cd->n_pins; ++k) {
            p = &cd->pins[k];
            if (p->is_under_pointer) {
                // Delete all wires connected to this pin
                for (int l = n_wires - 1; l >= 0; --l) {
                    wd = &hd->wire_descriptions[l];
                    if ((cd->number == wd->c1 && p->number == wd->c1_pin) || (cd->number == wd->c2 && p->number == wd->c2_pin)) {
                        hd->changed = 1;
                        if (l < n_wires - 1) {
                            memmove(wd, &hd->wire_descriptions[l + 1], sizeof *wd * (n_wires - 1 - l));
                            memset(&hd->wire_descriptions[n_wires - 1], 0, sizeof *wd);
                        }
                        --n_wires;
                    }
                }
                if (n_wires < hd->n_wire_descriptions) {
                    hd->n_wire_descriptions = n_wires;
                    if (n_wires > 0) {
                        void *mem = realloc(hd->wire_descriptions, sizeof *hd->wire_descriptions * hd->n_wire_descriptions);
                        if (mem == NULL) {
                            fprintf(stderr, "Error reallocating memory\n");
                        }
                        hd->wire_descriptions = mem;
                    } else {
                        free(hd->wire_descriptions);
                        hd->wire_descriptions = NULL;
                    }
                }
                // Handled this pin
                p->is_under_pointer = 0;
            }
        }
    }

    return;
}

void try_to_add_wire(program_state_t *state)
{
    harness_description_t *hd = &state->harness_descriptions[state->harness_index];
    wire_description_t *wd = NULL;
    if (state->p1_under_pointer == NULL || state->p2_under_pointer == NULL) {
        return;
    }
    int c1 = state->c1_under_pointer->number;
    int p1 = state->p1_under_pointer->number;
    int c2 = state->c2_under_pointer->number;
    int p2 = state->p2_under_pointer->number;
    int wire_exists = 0;
    for (int i = 0; i < hd->n_wire_descriptions; ++i) {
        wd = &hd->wire_descriptions[i];
        if ((((wd->c1 == c1 && wd->c1_pin == p1) && (wd->c2 == c2 && wd->c2_pin == p2)) || ((wd->c1 == c2 && wd->c1_pin == p2) && (wd->c2 == c1 && wd->c2_pin == p1)))) {
            wire_exists = 1;
        }
    }
    if (!wire_exists) {
        void *mem = realloc(hd->wire_descriptions, sizeof *hd->wire_descriptions * (hd->n_wire_descriptions + 1));
        if (mem == NULL) {
            fprintf(stderr, "Error allocating memory\n");
            return;
        }
        hd->wire_descriptions = mem;
        hd->n_wire_descriptions++;
        wd = &hd->wire_descriptions[hd->n_wire_descriptions - 1];
        wd->c1 = c1;
        wd->c1_pin = p1;
        wd->c2 = c2;
        wd->c2_pin = p2;
        wd->colour = hd->default_wire_colour;
        wd->length = hd->default_wire_length;
        wd->gauge = hd->default_wire_gauge;
        wd->thickness = DEFAULT_WIRE_THICKNESS;
        wd->straight_fraction = DEFAULT_WIRE_STRAIGHT_FRACTION;
        wd->is_highlighted = 0;
    }
    reset_pin_under_pointer_states(state);
}
    
void draw_pin_to_pointer(program_state_t *state)
{
    float harness_x_span = state->mouse_position.x - state->wire_drawing_first_end.x;
    float dx = DEFAULT_WIRE_STRAIGHT_FRACTION * harness_x_span;
    Vector2 offset1 = {state->wire_drawing_first_end.x + dx, state->wire_drawing_first_end.y};
    Vector2 offset2 = {state->mouse_position.x - dx, state->mouse_position.y};
    Vector2 points[] = {state->wire_drawing_first_end, offset1, offset2, state->mouse_position};
    DrawSplineBezierCubic(points, 4, (DEFAULT_WIRE_THICKNESS + 4.5) * state->zoom_level, get_color_from_string(DEFAULT_HIGHLIGHT_COLOR));
    DrawSplineBezierCubic(points, 4, (DEFAULT_WIRE_THICKNESS) * state->zoom_level, state->foreground_color);

    return;
}

void reset_pin_under_pointer_states(program_state_t *state)
{
    // Is a pin under the mouse pointer?

    connector_t *c = NULL;
    connector_description_t *cd = NULL;
    wire_description_t *wd = NULL;
    pin_t *p = NULL;
    int yoff = 0;
    int xoff = 0;

    harness_t *h = &state->harnesses[state->harness_index];
    harness_description_t *hd = &state->harness_descriptions[state->harness_index];
    int n_wires = hd->n_wire_descriptions;
    for (int j = 0; j < hd->n_connector_descriptions; ++j) {
        cd = &hd->connector_descriptions[j];
        for (int k = 0; k < cd->n_pins; ++k) {
            cd->pins[k].is_under_pointer = 0;
        }
    }
    state->n_pins_under_pointer = 0;
    state->c1_under_pointer = NULL;
    state->p1_under_pointer = NULL;
    state->c2_under_pointer = NULL;
    state->p2_under_pointer = NULL;

    return;
}

