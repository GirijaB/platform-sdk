#include <airmap/cmds/airmap/cmd/start_flight_comms.h>

#include <airmap/client.h>
#include <airmap/codec.h>
#include <airmap/context.h>
#include <airmap/util/telemetry_simulator.h>

#include <fstream>
#include <iterator>
#include <thread>

namespace cli = airmap::util::cli;
namespace cmd = airmap::cmds::airmap::cmd;

using json = nlohmann::json;

cmd::StartFlightComms::StartFlightComms()
    : cli::CommandWithFlagsAndAction{cli::Name{"start-flight-comms"},
                                     cli::Usage{"prepare flight for injection of telemetry data"},
                                     cli::Description{"prepare flight for injection of telemetry data"}} {
  flag(cli::make_flag(cli::Name{"api-key"}, cli::Description{"api-key for authenticating with the AirMap services"},
                      params_.api_key));
  flag(cli::make_flag(cli::Name{"authorization"},
                      cli::Description{"token used for authorizing with the AirMap services"}, params_.authorization));
  flag(cli::make_flag(cli::Name{"flight-id"}, cli::Description{"telemetry is sent for this flight id"},
                      params_.flight_id));

  action([this](const cli::Command::Context& ctxt) {
    auto result = ::airmap::Context::create(create_default_logger());

    if (!result) {
      ctxt.cout << "Could not acquire resources for accessing AirMap services" << std::endl;
      return 1;
    }

    auto context = result.value();

    context->create_client_with_credentials(
        Client::Credentials{params_.api_key},
        [this, &ctxt, context](const ::airmap::Context::ClientCreateResult& result) {
          if (not result)
            return;

          auto client = result.value();

          client->flights().start_flight_communications(
              Flights::StartFlightCommunications::Parameters{params_.authorization, params_.flight_id},
              [this, &ctxt, context, client](const Flights::StartFlightCommunications::Result& result) {
                if (!result) {
                  ctxt.cout << "Failed to start flight communications" << std::endl;
                  context->stop();
                  return;
                }

                ctxt.cout << "Started flight communications: " << result.value().key << std::endl;
                context->stop();
              });
        });

    context->run();
    return 0;
  });
}