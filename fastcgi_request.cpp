#include <sys/uio.h>

#include "fastcgi_request.h"

#include "fastcgi_conn.h"
#include "fastcgi_parse.h"

FastCGIRequest::FastCGIRequest(uint16_t request_id, FastCGIConn* conn)
		: request_id_(request_id),
		  conn_(conn),
		  out_buf_(fastcgi_max_record_len) {}

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
	CHECK(!body_sent_);
	CHECK(out_buf_.Write(name));
	CHECK(out_buf_.Write(": "));
	CHECK(out_buf_.Write(value));
	CHECK(out_buf_.Write("\n"));
}

void FastCGIRequest::WriteBody(const std::string_view& body) {
	if (!body_sent_) {
		CHECK(out_buf_.Write("\n"));
		body_sent_ = true;
	}
	// TODO: make this able to span multiple packets
	CHECK(out_buf_.Write(body));
}

void FastCGIRequest::End() {
	FastCGIHeader output_header;
	FastCGIHeader end_header;
	FastCGIEndRequest end;

	const auto output_len = out_buf_.ReadMaxLen();

	output_header.version = 1;
	output_header.type = 6;
	output_header.SetRequestId(request_id_);
	output_header.SetContentLength(output_len);

	end_header.version = 1;
	end_header.type = 3;
	end_header.SetRequestId(request_id_);
	end_header.SetContentLength(sizeof(end));

	std::vector<iovec> vecs{
		iovec{
			.iov_base = &output_header,
			.iov_len = sizeof(output_header),
		},
		iovec{
			.iov_base = (void *)(CHECK_NOTNULL(out_buf_.Read(output_len))),
			.iov_len = output_len,
		},
		{
			.iov_base = &end_header,
			.iov_len = sizeof(end_header),
		},
		{
			.iov_base = &end,
			.iov_len = sizeof(end),
		},
	};
	conn_->Write(vecs);
}
