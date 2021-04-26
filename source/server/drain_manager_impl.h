#pragma once

#include <functional>

#include "envoy/common/time.h"
#include "envoy/config/listener/v3/listener.pb.h"
#include "envoy/event/timer.h"
#include "envoy/server/drain_manager.h"
#include "envoy/server/instance.h"

#include "common/common/logger.h"

namespace Envoy {
namespace Server {

/**
 * Implementation of drain manager that does the following by default:
 * 1) Terminates the parent process after 15 minutes.
 * 2) Drains the parent process over a period of 10 minutes where drain close becomes more
 *    likely each second that passes.
 */
class DrainManagerImpl : Logger::Loggable<Logger::Id::main>, public DrainManager {
public:
  DrainManagerImpl(Instance& server, envoy::config::listener::v3::Listener::DrainType drain_type);

  // Network::DrainDecision
  bool drainClose() const override;

  // Server::DrainManager
  void startDrainSequence(std::function<void()> drain_complete_cb) override;
  bool draining() const override { return draining_; }
  void startParentShutdownSequence() override;

  // Draining connections rather than listeners, draining connections has the effect of
  // limiting downstream connections once in effect.
  // Create a separate class for this?
  bool drainingConnections() const override { return drain_connections_limit_ != 0; }
  bool setConnectionDrainPercentage(uint16_t percentage_of_connections) override;
  uint64_t drainConnectionsLimit() const override { return drain_connections_limit_;}

private:
  Instance& server_;
  const envoy::config::listener::v3::Listener::DrainType drain_type_;

  std::atomic<bool> draining_{false};
  Event::TimerPtr drain_tick_timer_;
  MonotonicTime drain_deadline_;

  Event::TimerPtr parent_shutdown_timer_;

  // When draining connections, there is a minimum water mark that is set based on the
  // requested percentage to drain.  If the drain_connections_limit_ is 0 then no connection
  // drain is in effect.
  std::atomic<uint64_t> drain_connections_limit_{};
//  std::atomic<bool> draining_connections_{false};
};

} // namespace Server
} // namespace Envoy
