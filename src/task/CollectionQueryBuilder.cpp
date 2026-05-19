#include "task/CollectionQueryBuilder.h"

#include "util/JsonUtil.h"
#include "util/TimeUtil.h"

#include <sstream>
#include <stdexcept>

namespace task {

std::string makeCid(const std::string& deviceName) {
    return deviceName + "-" + std::to_string(util::nowMilliseconds());
}

std::string buildQuery(const std::string& cid, const std::vector<DataPoint>& points) {
    if (points.empty()) {
        throw std::runtime_error("query construct failed: no DataPoint");
    }

    std::ostringstream query;
    query << "{\"cid\":\"" << util::escapeJson(cid) << "\",\"query\":[";
    for (std::size_t i = 0; i < points.size(); ++i) {
        if (i > 0) {
            query << ",";
        }
        query << "\"" << util::escapeJson(points[i].namePath) << "\"";
    }
    query << "]}";
    return query.str();
}

} // namespace task
