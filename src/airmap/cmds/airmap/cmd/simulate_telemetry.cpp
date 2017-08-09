#include <airmap/cmds/airmap/cmd/simulate_telemetry.h>

#include <airmap/client.h>
#include <airmap/codec.h>
#include <airmap/context.h>
#include <airmap/util/formatting_logger.h>
#include <airmap/util/telemetry_simulator.h>

#include <fstream>
#include <iterator>
#include <thread>

namespace cli = airmap::util::cli;
namespace cmd = airmap::cmds::airmap::cmd;

using json = nlohmann::json;

namespace {

auto polygon =
    airmap::Geometry::polygon({airmap::Geometry::Coordinate{47.37708083985247, 8.546290397644043,
                                                            airmap::Optional<double>{}, airmap::Optional<double>{}},
                               airmap::Geometry::Coordinate{47.37629618703235, 8.545989990234375,
                                                            airmap::Optional<double>{}, airmap::Optional<double>{}},
                               airmap::Geometry::Coordinate{47.37667398429217, 8.548049926757812,
                                                            airmap::Optional<double>{}, airmap::Optional<double>{}},
                               airmap::Geometry::Coordinate{47.37708083985247, 8.546290397644043,
                                                            airmap::Optional<double>{}, airmap::Optional<double>{}}});

constexpr const char* component{"telemetry-simulator"};
}  // namespace

cmd::SimulateTelemetry::SimulateTelemetry()
    : cli::CommandWithFlagsAndAction{cli::Name{"simulate-telemetry"},
                                     cli::Usage{"inject artificial telemetry data for a given flight"},
                                     cli::Description{"inject artificial telemetry data for a given flight"}} {
  flag(cli::make_flag(cli::Name{"api-key"}, cli::Description{"api-key for authenticating with the AirMap services"},
                      params_.api_key));
  flag(cli::make_flag(cli::Name{"authorization"},
                      cli::Description{"token used for authorizing with the AirMap services"}, params_.authorization));
  flag(cli::make_flag(cli::Name{"encryption-key"}, cli::Description{"key used for encrypting telemetry messages"},
                      params_.encryption_key));
  flag(cli::make_flag(cli::Name{"host"}, cli::Description{"telemetry host address"}, params_.host));
  flag(cli::make_flag(cli::Name{"port"}, cli::Description{"telemetry host port"}, params_.port));
  flag(cli::make_flag(cli::Name{"frequency"}, cli::Description{"telemetry is sent with `FREQUENCY` Hz"},
                      params_.frequency));
  flag(cli::make_flag(cli::Name{"flight-id"}, cli::Description{"telemetry is sent for this flight id"},
                      params_.flight.id));
  flag(cli::make_flag(cli::Name{"geometry-file"}, cli::Description{"use the polygon defined in this geojson file"},
                      params_.geometry_file));

  action([this](const cli::Command::Context& ctxt) {
    log_ = util::FormattingLogger{create_default_logger()};

    if (!params_.api_key) {
      log_.errorf(component, "missing parameter 'api-key'");
      return 1;
    }

    if (!params_.api_key.get().validate()) {
      log_.errorf(component, "parameter 'api-key' for accessing AirMap services must not be empty");
      return 1;
    }

    if (!params_.authorization) {
      log_.errorf(component, "missing parameter 'authorization'");
      return 1;
    }

    if (!params_.authorization.get().validate()) {
      log_.errorf(component, "parameter 'authorization' for accessing AirMap services must not be empty");
      return 1;
    }

    if (!params_.host) {
      log_.errorf(component, "missing parameter 'host'");
      return 1;
    }

    if (!params_.host.get().validate()) {
      log_.errorf(component, "parameter 'host' must not be empty");
      return 1;
    }

    if (!params_.encryption_key) {
      log_.errorf(component, "missing parameter 'encryption-key'");
      return 1;
    }

    if (!params_.encryption_key.get().validate()) {
      log_.errorf(component, "parameter 'encryption-key' must not be empty");
      return 1;
    }

    auto result = ::airmap::Context::create(log_.logger());

    if (!result) {
      ctxt.cout << "Could not acquire resources for accessing AirMap services" << std::endl;
      return 1;
    }

    auto context = result.value();

    ctxt.cout << "Sending telemetry package to" << std::endl
              << "  host:      " << params_.host.get() << std::endl
              << "  port:      " << params_.port << std::endl
              << "  frequency: " << params_.frequency << std::endl
              << "  flight-id: " << params_.flight.id << std::endl
              << "  api-key:   " << params_.api_key.get() << std::endl
              << "  enc-key:   " << params_.encryption_key.get() << std::endl;

    ::setenv("AIRMAP_TELEMETRY_HOST", params_.host.get().string().c_str(), 1);
    ::setenv("AIRMAP_TELEMETRY_PORT", boost::lexical_cast<std::string>(params_.port).c_str(), 1);

    auto geometry = polygon;

    if (params_.geometry_file) {
      std::ifstream in{params_.geometry_file.get()};
      if (!in) {
        log_.errorf(component, "failed to open %s for reading", params_.geometry_file.get());
        return 1;
      }
      geometry = json::parse(in);
    }

    context->create_client_with_credentials(
        Client::Credentials{params_.api_key.get()},
        [this, &ctxt, context, geometry](const ::airmap::Context::ClientCreateResult& result) {
          if (not result)
            return;

          auto client = result.value();

          std::thread submitter{[this, &ctxt, geometry, context, client]() {
            util::TelemetrySimulator simulator{geometry.details_for_polygon()};

            while (true) {
              auto data = simulator.update();

              log_.infof(component, "Submitting update for position (%f,%f)", data.latitude, data.longitude);

              client->telemetry().submit_updates(
                  params_.flight, params_.encryption_key.get(),
                  {Telemetry::Update{Telemetry::Position{milliseconds_since_epoch(Clock::universal_time()),
                                                         data.latitude, data.longitude, 100, 100, 2}}});
              std::this_thread::sleep_for(std::chrono::milliseconds{1000 / params_.frequency});
            }
          }};
          submitter.detach();
        });

    context->run();
    return 0;
  });
}