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
        std::set< std::string >& arrayElements
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
        std::stack< bool > isArray;
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
                        if (
                            !isArray.empty()
                            && isArray.top()
                        ) {
                            if (parent.GetType() != Json::Value::Type::Array) {
                                parent = Json::Array({});
                            }
                            parent.Add(Json::Value());
                            auto& child = parent[parent.GetSize() - 1];
                            elements.push(&child);
                        } else {
                            if (parent.GetType() != Json::Value::Type::Object) {
                                parent = Json::Object({});
                            }
                            auto& child = parent[data];
                            elements.push(&child);
                        }
                        isArray.push(arrayElements.find(data) != arrayElements.end());
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
                            isArray.pop();
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
                    const auto parsedBody = XmlToJson(
                        transaction->response.body,
                        std::set< std::string >({"Buckets"})
                    );
                    result.owner.id = parsedBody["Owner"]["ID"];
                    result.owner.displayName = parsedBody["Owner"]["DisplayName"];
                    const auto& buckets = parsedBody["Buckets"];
                    const auto numBuckets = buckets.GetSize();
                    for (size_t i = 0; i < numBuckets; ++i) {
                        const auto& bucketJson = buckets[i];
                        Bucket bucket;
                        bucket.name = bucketJson["Name"];
                        bucket.creationDate = ParseTimestamp(bucketJson["CreationDate"]);
                        result.buckets.push_back(std::move(bucket));
                    }
                }
                return result;
            }
        );
    }

}
