#include "request.h"

#include "../util.h"

namespace http {

Request::Request() : Request("", "", 0) {
}

Request::Request(const std::string& method, const std::string& path, int minorversion)
    : Message("HTTP/1." + std::to_string(minorversion)), method(method), pathwithqueryparams(path)
{
  size_t queryParamSeparator = path.find("?");
  if (queryParamSeparator != std::string::npos) {
    this->path = util::urlDecode(path.substr(0, queryParamSeparator));
    parseQueryParams(path.substr(queryParamSeparator + 1));
  }
  else {
    this->path = util::urlDecode(path);
  }
}

void Request::parseQueryParams(const std::string& queryparams) {
  size_t pos = 0;
  while (pos <= queryparams.size()) {
    size_t paramseparator = queryparams.find("&", pos);
    std::string pair = paramseparator == std::string::npos ?
        queryparams.substr(pos) : queryparams.substr(pos, paramseparator - pos);
    size_t valueseparator = pair.find("=");
    if (valueseparator != std::string::npos) {
      std::string name = util::urlDecode(pair.substr(0, valueseparator));
      this->queryparams[name] = util::urlDecode(pair.substr(valueseparator + 1));
    }
    if (paramseparator == std::string::npos) {
      break;
    }
    pos = paramseparator + 1;
  }
}

std::vector<char> Request::serialize() const {
  return serializeCommon(method + " " + pathwithqueryparams + " " + protocol);
}

std::string Request::getMethod() const {
  return method;
}

std::string Request::getPath() const {
  return path;
}

std::string Request::getPathWithQueryParams() const {
  return pathwithqueryparams;
}

void Request::setMethod(const std::string& method) {
  this->method = method;
}

void Request::setPath(const std::string& path) {
  this->path = path;
  generatePathWithQueryParams();
}

void Request::addQueryParam(const std::string& queryparam, const std::string& value) {
  queryparams[queryparam] = value;
  generatePathWithQueryParams();
}

bool Request::hasQueryParam(const std::string& queryparam) const {
  return queryparams.find(queryparam) != queryparams.end();
}

void Request::removeQueryParam(const std::string& queryParam) {
  queryparams.erase(queryParam);
  generatePathWithQueryParams();
}

std::string Request::getQueryParamValue(const std::string& queryparam) const {
  std::map<std::string, std::string>::const_iterator it = queryparams.find(queryparam);
  if (it != queryparams.end()) {
    return it->second;
  }
  return "";
}

std::map<std::string, std::string> Request::getQueryParams() const {
  return queryparams;
}

void Request::generatePathWithQueryParams() {
  pathwithqueryparams = path;
  for (std::map<std::string, std::string>::const_iterator it = queryparams.begin(); it != queryparams.end(); ++it) {
    pathwithqueryparams.append((it == queryparams.begin() ? "?" : "&") + it->first + "=" + it->second);
  }
}
} // namespace EWHTTPParser
