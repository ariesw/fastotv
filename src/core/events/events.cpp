#include "core/events/events.h"

#include <common/time.h>

namespace core {
namespace events {

TimeInfo::TimeInfo() : time_millisecond(common::time::current_mstime()) {}
}
}