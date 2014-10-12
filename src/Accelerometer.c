#include <pebble.h>

#define TEXT_BUFFER_SIZE 256
#define SAMPLE_RATE ACCEL_SAMPLING_10HZ

#define OUTPUT_FIXED(X) (int16_t)(X/1000) , (int16_t)(abs(X) % 1000)

static Window *window;
static TextLayer *mainTextLayer = NULL;
static TextLayer *lightInfoTextLayer = NULL;
static char textBuffer[TEXT_BUFFER_SIZE];
static bool sampling = false;
static bool light = false;

static uint16_t sqrtFixed(uint32_t value) {
    uint16_t result = 0, ub = 0xFFFF, lb = 0;
    uint32_t result2;

    while (true) {
        result = ((uint32_t)ub + (uint32_t)lb) / 2;

        if (result == ub || result == lb) break;

        result2 = (uint32_t)result * (uint32_t)result;   

        if      (result2 > value) ub = result;
        else if (result2 < value) lb = result;
        else break;
    }

    return result;
}

static void updateTextBuffer(AccelData* accelerometerData, bool force) {
    if (!sampling && !force) return;

    int32_t     x = accelerometerData->x,
                y = accelerometerData->y,
                z = accelerometerData->z,
                modulus = sqrtFixed(x*x + y*y + z*z);

    snprintf(textBuffer, TEXT_BUFFER_SIZE,
        "Accelerometer data:\n\nX: %i.%i\nY: %i.%i\nZ: %i.%i\n\nmodulus: %i.%i",
        OUTPUT_FIXED(x), OUTPUT_FIXED(y), OUTPUT_FIXED(z), OUTPUT_FIXED(modulus)
    );

    if (mainTextLayer) layer_mark_dirty(text_layer_get_layer(mainTextLayer));;
}

static void updateLightInfo() {
    if (lightInfoTextLayer) text_layer_set_text(lightInfoTextLayer,
        light ? "light: on" : "light: auto"
    );
}

static void accel_data_handler(AccelData* accelerometerData, uint32_t numSamples) {
    if (numSamples > 0) updateTextBuffer(accelerometerData, false);
}

static void selectDownHandler(ClickRecognizerRef recognizer, void* context) {
    sampling = true;
}

static void selectUpHandler(ClickRecognizerRef recognizer, void* context) {
    sampling = false;
}

static void upClickHandler (ClickRecognizerRef recognizer, void* context) {
    light = !light;
    light_enable(light);
    updateLightInfo();
}

static void downClickHandler (ClickRecognizerRef recognizer, void* context) {
    sampling = !sampling;
}

static void clickConfigProvider(void* context) {
    window_raw_click_subscribe(BUTTON_ID_SELECT, selectDownHandler, selectUpHandler, NULL);
    window_single_click_subscribe(BUTTON_ID_UP, upClickHandler);
    window_single_click_subscribe(BUTTON_ID_DOWN, downClickHandler);
}

static void window_load(Window *window) {
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    mainTextLayer = text_layer_create((GRect) {
        .origin = { 5, 10 },
        .size = {bounds.size.w - 5, 100 }
    });

    text_layer_set_text(mainTextLayer, textBuffer);
    text_layer_set_text_alignment(mainTextLayer, GTextAlignmentLeft);

    lightInfoTextLayer = text_layer_create((GRect) {
        .origin = { 5, 120 },
        .size = {bounds.size.w - 5, 20 }
    });

    text_layer_set_text_alignment(lightInfoTextLayer, GTextAlignmentLeft);
    updateLightInfo();

    layer_add_child(window_layer, text_layer_get_layer(mainTextLayer));
    layer_add_child(window_layer, text_layer_get_layer(lightInfoTextLayer));
}

static void window_unload(Window *window) {
    text_layer_destroy(mainTextLayer);
    mainTextLayer = NULL;

    text_layer_destroy(lightInfoTextLayer);
    lightInfoTextLayer = NULL;
}

static void init(void) {
    accel_data_service_subscribe(1, accel_data_handler);
    accel_service_set_sampling_rate(SAMPLE_RATE);

    static AccelData accelerometerData;
    accel_service_peek(&accelerometerData);
    updateTextBuffer(&accelerometerData, true);

    window = window_create();

    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });

    window_set_click_config_provider(window, clickConfigProvider);

    window_stack_push(window, true);
}

static void deinit(void) {
    accel_data_service_unsubscribe();
    window_destroy(window);
}

int main(void) {
    init();

    APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

    app_event_loop();
    deinit();
}
