#include "fastcgi_parse.h"

FastCGIHeader::FastCGIHeader(uint8_t type_in, uint16_t request_id, uint16_t content_length)
		: type(type_in) {
	SetRequestId(request_id);
	SetContentLength(content_length);
}
