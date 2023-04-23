// Checks various trigger setting manipulation on the viewworks camera
#include "acquire.h"
#include "device/hal/device.manager.h"
#include "logger.h"
#include <cstdio>
#include <stdexcept>

void
reporter(int is_error,
         const char* file,
         int line,
         const char* function,
         const char* msg)
{
    fprintf(is_error ? stderr : stdout,
            "%s%s(%d) - %s: %s\n",
            is_error ? "ERROR " : "",
            file,
            line,
            function,
            msg);
}

/// Helper for passing size static strings as function args.
/// For a function: `f(char*,size_t)` use `f(SIZED("hello"))`.
/// Expands to `f("hello",5)`.
#define SIZED(str) str, sizeof(str)

#define L (aq_logger)
#define LOG(...) L(0, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define ERR(...) L(1, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#define EXPECT(e, ...)                                                         \
    do {                                                                       \
        if (!(e)) {                                                            \
            char buf[1 << 8] = { 0 };                                          \
            ERR(__VA_ARGS__);                                                  \
            snprintf(buf, sizeof(buf) - 1, __VA_ARGS__);                       \
            throw std::runtime_error(buf);                                     \
        }                                                                      \
    } while (0)
#define CHECK(e) EXPECT(e, "Expression evaluated as false: %s", #e)
#define DEVOK(e) CHECK(Device_Ok == (e))
#define OK(e) CHECK(AcquireStatus_Ok == (e))

int
main()
{
    AcquireRuntime* runtime = 0;
    try {
        {
            runtime = acquire_init(reporter);
            auto dm = acquire_device_manager(runtime);
            CHECK(runtime);
            CHECK(dm);

            AcquireProperties props = {};
            OK(acquire_get_configuration(runtime, &props));

            DEVOK(device_manager_select(dm,
                                        DeviceKind_Camera,
                                        SIZED("VIEWORKS.*") - 1,
                                        &props.video[0].camera.identifier));
            DEVOK(device_manager_select(dm,
                                        DeviceKind_Storage,
                                        SIZED("Trash") - 1,
                                        &props.video[0].storage.identifier));

            OK(acquire_configure(runtime, &props));

            AcquirePropertyMetadata metadata = { 0 };
            OK(acquire_get_configuration_metadata(runtime, &metadata));

            props.video[0].camera.settings.binning = 1;
            props.video[0].camera.settings.pixel_type = SampleType_u12;
            props.video[0].camera.settings.shape = {
                .x = (uint32_t)metadata.video[0].camera.shape.x.high,
                .y = (uint32_t)metadata.video[0].camera.shape.y.high,
            };
            props.video[0].camera.settings.exposure_time_us = 1e4;
            props.video[0].max_frame_count = 1000;

            CHECK(
              props.video[0].camera.settings.input_triggers.frame_start.kind ==
              Signal_Input);
            props.video[0].camera.settings.input_triggers.frame_start.edge =
              TriggerEdge_Rising;
            props.video[0].camera.settings.input_triggers.frame_start.line = 0;
            props.video[0].camera.settings.input_triggers.frame_start.enable =
              1;

            OK(acquire_configure(runtime, &props));
            CHECK(
              props.video[0].camera.settings.input_triggers.frame_start.line ==
              0);
            CHECK(props.video[0]
                    .camera.settings.input_triggers.frame_start.enable == 1);

            props.video[0].camera.settings.input_triggers.frame_start.line = 1;

            OK(acquire_configure(runtime, &props));
            CHECK(
              props.video[0].camera.settings.input_triggers.frame_start.line ==
              1);
            CHECK(props.video[0]
                    .camera.settings.input_triggers.frame_start.enable == 1);

            props.video[0].camera.settings.input_triggers.frame_start.enable =
              0;

            OK(acquire_configure(runtime, &props));
            CHECK(
              props.video[0].camera.settings.input_triggers.frame_start.line ==
              1);
            CHECK(props.video[0]
                    .camera.settings.input_triggers.frame_start.enable == 0);

            props.video[0].camera.settings.input_triggers.frame_start.line = 0;
            props.video[0].camera.settings.input_triggers.frame_start.enable =
              1;

            OK(acquire_configure(runtime, &props));
            CHECK(
              props.video[0].camera.settings.input_triggers.frame_start.line ==
              0);
            CHECK(props.video[0]
                    .camera.settings.input_triggers.frame_start.enable == 1);

            OK(acquire_shutdown(runtime));
        }

        // read back where we left it
        // triggers should be disabled
        {
            auto runtime = acquire_init(reporter);
            auto dm = acquire_device_manager(runtime);
            CHECK(runtime);
            CHECK(dm);

            AcquireProperties props = {};
            AcquirePropertyMetadata meta = {};

            DEVOK(device_manager_select(dm,
                                        DeviceKind_Camera,
                                        SIZED("VIEWORKS.*") - 1,
                                        &props.video[0].camera.identifier));
            DEVOK(device_manager_select(dm,
                                        DeviceKind_Storage,
                                        SIZED("Trash") - 1,
                                        &props.video[0].storage.identifier));

            OK(acquire_configure(runtime, &props));
            OK(acquire_get_configuration_metadata(runtime, &meta));

            CHECK(props.video[0]
                    .camera.settings.input_triggers.frame_start.enable == 0);

            OK(acquire_shutdown(runtime));
        }
        LOG("OK");
        return 0;
    } catch (const std::runtime_error& e) {
        ERR("Runtime error: %s", e.what());

    } catch (...) {
        ERR("Uncaught exception");
    }
    acquire_shutdown(runtime);
    return 1;
}
