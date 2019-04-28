#include <sys/uio.h>

#include "fastcgi_request.h"

#include "fastcgi_conn.h"

namespace {

template<class T> void AppendVec(const T& obj, std::vector<iovec>* vec) {
	vec->push_back(iovec{
		.iov_base = (void*)(&obj),
		.iov_len = sizeof(obj),
	});
}

} // namespace

FastCGIRequest::FastCGIRequest(uint16_t request_id, FastCGIConn* conn)
		: request_id_(request_id),
		  conn_(conn),
		  out_buf_(fastcgi_max_record_len) {}

uint16_t FastCGIRequest::RequestId() {
	return request_id_;
}

void FastCGIRequest::AddParam(const std::string_view& key, const std::string_view& value) {
	params_.try_emplace(std::string(key), std::string(value));
}

void FastCGIRequest::AddIn(const std::string_view& in) {
	in_.append(in);
}

const std::string& FastCGIRequest::GetParam(const std::string& key) {
	return params_.at(key);
}

void FastCGIRequest::WriteHeader(const std::string_view& name, const std::string_view& value) {
	CHECK(!body_written_);
	CHECK(out_buf_.Write(name));
	CHECK(out_buf_.Write(": "));
	CHECK(out_buf_.Write(value));
	CHECK(out_buf_.Write("\n"));
}

void FastCGIRequest::WriteBody(const std::string_view& body) {
	if (!body_written_) {
		CHECK(out_buf_.Write("\n"));
		body_written_ = true;
	}
	// TODO: make this able to span multiple packets
	CHECK(out_buf_.Write(body));
}

void FastCGIRequest::Flush() {
	std::vector<iovec> vecs;

	auto header = OutputHeader();
	AppendVec(header, &vecs);

	vecs.push_back(OutputVec());

	conn_->Write(vecs);
	out_buf_.Commit();
}

void FastCGIRequest::End() {
	// Fully empty response not allowed
	WriteBody("");

	std::vector<iovec> vecs;

	// Must be outside if block, so it lives through Write() below
	auto output_header = OutputHeader();
	if (output_header.ContentLength()) {
		AppendVec(output_header, &vecs);
		vecs.push_back(OutputVec());
	}

	FastCGIEndRequest end;
	FastCGIHeader end_header(3, request_id_, sizeof(end));
	AppendVec(end_header, &vecs);
	AppendVec(end, &vecs);

	conn_->Write(vecs);
}

iovec FastCGIRequest::OutputVec() {
	const auto output_len = out_buf_.ReadMaxLen();
	return iovec{
		.iov_base = (void *)(CHECK_NOTNULL(out_buf_.Read(output_len))),
		.iov_len = output_len,
	};
}

FastCGIHeader FastCGIRequest::OutputHeader() {
	return FastCGIHeader(6, request_id_, out_buf_.ReadMaxLen());
}
