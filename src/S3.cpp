/**
 * @file S3.cpp
 *
 * This module contains the implementation of the Aws::S3 class.
 *
 * © 2019 by Richard Walters
 */

#include <Aws/S3.hpp>
#include <Aws/SignApi.hpp>
#include <future>
#include <Json/Value.hpp>
#include <set>
#include <stack>
#include <stdio.h>
#include <string>
#include <SystemAbstractions/StringExtensions.hpp>
#include <time.h>

namespace {

    /**
     * This function converts the given time from seconds since the UNIX epoch
     * to the ISO-8601 format YYYYMMDD'T'HHMMSS'Z' expected by AWS.
     *
     * @param[in] time
     *     This is the time in seconds since the UNIX epoch.
     *
     * @return
     *     The given time, formatted in the ISO-8601 format
     *     YYYYMMDD'T'HHMMSS'Z', is returned.
     */
    std::string AmzTimestamp(time_t time) {
        char buffer[17];
        (void)strftime(buffer, sizeof(buffer), "%Y%m%dT%H%M%SZ", gmtime(&time));
        return buffer;
    }

    /**
     * Breaks the given string at each instance of the given delimiter,
     * returning the pieces as a collection of substrings.  The delimiter
     * characters are removed.
     *
     * @param[in] s
     *     This is the string to split.
     *
     * @param[in] d
     *     This is the delimiter character at which to split the string.
     *
     * @return
     *     The collection of substrings that result from breaking the given
     *     string at each delimiter character is returned.
     */
    std::vector< std::string > Split(
        const std::string& s,
        char d
    ) {
        std::vector< std::string > values;
        auto remainder = s;
        while (!remainder.empty()) {
            auto delimiter = remainder.find_first_of(d);
            if (delimiter == std::string::npos) {
                values.push_back(remainder);
                remainder.clear();
            } else {
                values.push_back(remainder.substr(0, delimiter));
                remainder = remainder.substr(delimiter + 1);
            }
        }
        return values;
    }

    /**
     * Convert the given time in UTC to the equivalent number of seconds
     * since the UNIX epoch (Midnight UTC January 1, 1970).
     *
     * @param[in] timestamp
     *     This is the timestamp to convert.
     *
     * @return
     *     The equivalent timetamp in seconds since the UNIX epoch
     *     is returned.
     */
    double ParseTimestamp(const std::string& timestamp) {
        int years, months, days, hours, minutes, seconds, milliseconds;
        (void)sscanf(
            timestamp.c_str(),
            "%d-%d-%dT%d:%d:%d.%dZ",
            &years,
            &months,
            &days,
            &hours,
            &minutes,
            &seconds,
            &milliseconds
        );
        static const auto isLeapYear = [](int year){
            if ((year % 4) != 0) { return false; }
            if ((year % 100) != 0) { return true; }
            return ((year % 400) == 0);
        };
        for (int yy = 1970; yy < years; ++yy) {
            seconds += (isLeapYear(yy) ? 366 : 365) * 86400;
        }
        static const int daysPerMonth[] = {
            31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
        };
        for (int mm = 1; mm < months; ++mm) {
            seconds += daysPerMonth[mm - 1] * 86400;
            if (
                (mm == 2)
                && isLeapYear(years)
            ) {
                seconds += 86400;
            }
        }
        seconds += (days - 1) * 86400;
        seconds += hours * 3600;
        seconds += minutes * 60;
        return (
            (double)seconds
            + (double)milliseconds / 1000.0
        );
    }

    /**
     * Convert the given XML document into the equivalent JSON.
     *
     * @param[in] xml
     *     This is the XML document to convert.
     *
     * @param[in] arrayElements
     *     This is the set of tags which should be interpreted as JSON
     *     arrays rather than as strings or objects.
     *
     * @return
     *     The equivalent JSON to the given XML document is returned.
     */
    Json::Value XmlToJson(
        const std::string& xml,
        const std::set< std::string >& arrayElements
    ) {
        auto json = Json::Object({});
        enum class State {
            Header,
            Document,
            TagBegin,
            Tag,
            TagBeginOrContent,
            Content,
            TagEnd,
            End,
        } state = State::Header;
        std::string data;
        std::stack< Json::Value* > elements;
        for (auto c: xml) {
            switch (state) {
                case State::Header: {
                    if (c == '>') {
                        state = State::Document;
                    }
                } break;

                case State::Document: {
                    if (c == '>') {
                        state = State::TagBegin;
                    }
                } break;

                case State::TagBegin: {
                    if (c == '<') {
                        state = State::Tag;
                        data.clear();
                    }
                } break;

                case State::Tag: {
                    if (c == '/') {
                        state = State::TagEnd;
                    } else if (c == '>') {
                        state = State::TagBeginOrContent;
                        auto& parent = (
                            elements.empty()
                            ? json
                            : *elements.top()
                        );
                        if (parent.GetType() != Json::Value::Type::Object) {
                            parent = Json::Object({});
                        }
                        if (arrayElements.find(data) == arrayElements.end()) {
                            auto& child = parent[data];
                            elements.push(&child);
                        } else {
                            if (parent[data].GetType() != Json::Value::Type::Array) {
                                parent[data] = Json::Array({});
                            }
                            auto& child = parent[data];
                            child.Add(Json::Value());
                            auto& element = child[child.GetSize() - 1];
                            elements.push(&element);
                        }
                        data.clear();
                    } else {
                        data += c;
                    }
                } break;

                case State::TagBeginOrContent: {
                    if (c == '<') {
                        state = State::Tag;
                    } else {
                        state = State::Content;
                        data += c;
                    }
                } break;

                case State::Content: {
                    if (c == '<') {
                        auto& parent = (
                            elements.empty()
                            ? json
                            : *elements.top()
                        );
                        parent = data;
                        state = State::TagEnd;
                    } else {
                        data += c;
                    }
                } break;

                case State::TagEnd: {
                    if (c == '>') {
                        if (elements.empty()) {
                            state = State::End;
                        } else {
                            elements.pop();
                            state = State::TagBegin;
                        }
                    }
                } break;

                default: break;
            }
        }
        return json;
    }

}

namespace Aws {

    /**
     * This contains the private properties of an S3 instance.
     */
    struct S3::Impl {
        /**
         * This is the HTTP client to use to communicate with Amazon S3.
         */
        std::shared_ptr< Http::IClient > http;

        /**
         * This is the Amazon Web Services (AWS) configuration to use.
         */
        Config config;
    };

    S3::~S3() noexcept = default;
    S3::S3(S3&& other) noexcept = default;
    S3& S3::operator=(S3&& other) noexcept = default;

    S3::S3()
        : impl_(new Impl)
    {
    }

    void S3::Configure(
        std::shared_ptr< Http::IClient > http,
        Config config
    ) {
        impl_->http = http;
        impl_->config = config;
    }

    auto S3::ListBuckets() -> std::future< ListBucketsResult > {
        auto impl(impl_);
        return std::async(
            std::launch::async,
            [impl]{
                ListBucketsResult result;
                const auto host = "s3." + impl->config.region + ".amazonaws.com";
                const auto date = AmzTimestamp(time(NULL));
                Http::Request request;
                request.method = "GET";
                request.target.SetHost(host);
                request.target.SetPort(443);
                request.target.SetPath({""});
                request.headers.AddHeader("Host", host);
                request.headers.AddHeader("x-amz-date", date);
                const auto canonicalRequest = SignApi::ConstructCanonicalRequest(request.Generate());
                const auto payloadHashOffset = canonicalRequest.find_last_of('\n') + 1;
                const auto payloadHash = canonicalRequest.substr(payloadHashOffset);
                const auto stringToSign = SignApi::MakeStringToSign(
                    impl->config.region,
                    "s3",
                    canonicalRequest
                );
                const auto authorization = SignApi::MakeAuthorization(
                    stringToSign,
                    canonicalRequest,
                    impl->config.accessKeyId,
                    impl->config.secretAccessKey
                );
                request.headers.AddHeader("Authorization", authorization);
                request.headers.AddHeader("x-amz-content-sha256", payloadHash);
                if (!impl->config.sessionToken.empty()) {
                    request.headers.AddHeader("x-amz-security-token", impl->config.sessionToken);
                }
                const auto rawRequest = request.Generate();
                const auto transaction = impl->http->Request(request);
                transaction->AwaitCompletion();
                result.transactionState = transaction->state;
                result.statusCode = transaction->response.statusCode;
                if (transaction->state == Http::IClient::Transaction::State::Completed) {
                    if (transaction->response.statusCode == 200) {
                        const auto parsedBody = XmlToJson(
                            transaction->response.body,
                            std::set< std::string >({"Bucket"})
                        );
                        result.owner.id = (std::string)parsedBody["Owner"]["ID"];
                        result.owner.displayName = (std::string)parsedBody["Owner"]["DisplayName"];
                        const auto& buckets = parsedBody["Buckets"]["Bucket"];
                        const auto numBuckets = buckets.GetSize();
                        for (size_t i = 0; i < numBuckets; ++i) {
                            const auto& bucketJson = buckets[i];
                            Bucket bucket;
                            bucket.name = (std::string)bucketJson["Name"];
                            bucket.creationDate = ParseTimestamp(bucketJson["CreationDate"]);
                            result.buckets.push_back(std::move(bucket));
                        }
                    } else {
                        result.errorInfo = XmlToJson(
                            transaction->response.body,
                            std::set< std::string >({})
                        );
                    }
                }
                return result;
            }
        );
    }

    auto S3::ListObjects(const std::string& bucketName) -> std::future< ListObjectsResult > {
        auto impl(impl_);
        return std::async(
            std::launch::async,
            [impl, bucketName]{
                ListObjectsResult result;
                const auto host = "s3." + impl->config.region + ".amazonaws.com";
                const auto date = AmzTimestamp(time(NULL));
                std::string continuationToken;
                do {
                    Http::Request request;
                    request.method = "GET";
                    request.target.SetHost(host);
                    request.target.SetPort(443);
                    request.target.SetPath({"", bucketName});
                    std::vector< std::string > queryParts = {"list-type=2"};
                    if (!continuationToken.empty()) {
                        queryParts.push_back("continuation-token=" + continuationToken);
                    }
                    request.target.SetQuery(SystemAbstractions::Join(queryParts, "&"));
                    request.headers.AddHeader("Host", host);
                    request.headers.AddHeader("x-amz-date", date);
                    const auto canonicalRequest = SignApi::ConstructCanonicalRequest(request.Generate());
                    const auto payloadHashOffset = canonicalRequest.find_last_of('\n') + 1;
                    const auto payloadHash = canonicalRequest.substr(payloadHashOffset);
                    const auto stringToSign = SignApi::MakeStringToSign(
                        impl->config.region,
                        "s3",
                        canonicalRequest
                    );
                    const auto authorization = SignApi::MakeAuthorization(
                        stringToSign,
                        canonicalRequest,
                        impl->config.accessKeyId,
                        impl->config.secretAccessKey
                    );
                    request.headers.AddHeader("Authorization", authorization);
                    request.headers.AddHeader("x-amz-content-sha256", payloadHash);
                    if (!impl->config.sessionToken.empty()) {
                        request.headers.AddHeader("x-amz-security-token", impl->config.sessionToken);
                    }
                    const auto rawRequest = request.Generate();
                    const auto transaction = impl->http->Request(request);
                    transaction->AwaitCompletion();
                    result.transactionState = transaction->state;
                    result.statusCode = transaction->response.statusCode;
                    if (transaction->state == Http::IClient::Transaction::State::Completed) {
                        if (transaction->response.statusCode == 200) {
                            const auto parsedBody = XmlToJson(
                                transaction->response.body,
                                std::set< std::string >({"Contents"})
                            );
                            const auto& parsedObjects = parsedBody["Contents"];
                            const auto numObjects = parsedObjects.GetSize();
                            for (size_t i = 0; i < numObjects; ++i) {
                                const auto& parsedObject = parsedObjects[i];
                                Object object;
                                object.key = (std::string)parsedObject["Key"];
                                object.lastModified = ParseTimestamp(parsedObject["LastModified"]);
                                object.eTag = (std::string)parsedObject["ETag"];
                                object.eTag = object.eTag.substr(6, object.eTag.size() - 12);
                                (void)sscanf(
                                    ((std::string)parsedObject["Size"]).c_str(),
                                    "%zu",
                                    &object.size
                                );
                                result.objects.push_back(std::move(object));
                            }
                            if ((std::string)parsedBody["IsTruncated"] == "true") {
                                continuationToken = (std::string)parsedBody["NextContinuationToken"];
                            } else {
                                continuationToken.clear();
                            }
                        } else {
                            result.errorInfo = XmlToJson(
                                transaction->response.body,
                                std::set< std::string >({})
                            );
                            break;
                        }
                    } else {
                        break;
                    }
                } while (!continuationToken.empty());
                return result;
            }
        );
    }

    auto S3::GetObject(
        const std::string& bucketName,
        const std::string& objectName
    ) -> std::future< GetObjectResult > {
        auto impl(impl_);
        return std::async(
            std::launch::async,
            [impl, bucketName, objectName]{
                GetObjectResult result;
                const auto host = "s3." + impl->config.region + ".amazonaws.com";
                const auto date = AmzTimestamp(time(NULL));
                Http::Request request;
                request.method = "GET";
                request.target.SetHost(host);
                request.target.SetPort(443);
                auto objectNameParts = Split(objectName, '/');
                objectNameParts.insert(objectNameParts.begin(), {"", bucketName});
                request.target.SetPath(objectNameParts);
                request.headers.AddHeader("Host", host);
                request.headers.AddHeader("x-amz-date", date);
                const auto canonicalRequest = SignApi::ConstructCanonicalRequest(request.Generate());
                const auto payloadHashOffset = canonicalRequest.find_last_of('\n') + 1;
                const auto payloadHash = canonicalRequest.substr(payloadHashOffset);
                const auto stringToSign = SignApi::MakeStringToSign(
                    impl->config.region,
                    "s3",
                    canonicalRequest
                );
                const auto authorization = SignApi::MakeAuthorization(
                    stringToSign,
                    canonicalRequest,
                    impl->config.accessKeyId,
                    impl->config.secretAccessKey
                );
                request.headers.AddHeader("Authorization", authorization);
                request.headers.AddHeader("x-amz-content-sha256", payloadHash);
                if (!impl->config.sessionToken.empty()) {
                    request.headers.AddHeader("x-amz-security-token", impl->config.sessionToken);
                }
                const auto rawRequest = request.Generate();
                const auto transaction = impl->http->Request(request);
                transaction->AwaitCompletion();
                result.transactionState = transaction->state;
                result.statusCode = transaction->response.statusCode;
                result.headers = transaction->response.headers;
                if (transaction->state == Http::IClient::Transaction::State::Completed) {
                    if (transaction->response.statusCode == 200) {
                        result.content = transaction->response.body;
                    } else {
                        result.errorInfo = XmlToJson(
                            transaction->response.body,
                            std::set< std::string >({})
                        );
                    }
                }
                return result;
            }
        );
    }

}
