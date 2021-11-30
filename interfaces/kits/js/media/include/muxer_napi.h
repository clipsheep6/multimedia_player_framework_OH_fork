#ifndef MUXER_NAPI_H
#define MUXER_NAPI_H

#include "muxer.h"
#include "media_errors.h"
#include "napi/native_api.h"

namespace OHOS {
namespace Media {
class MuxerNapi {
public:
	static napi_value Init(napi_env env, napi_value exports);
private:
	static napi_value Constructor(napi_env env, napi_callback_info info);
	static void Destructor(napi_env env, void* nativeObject, void* finalize);
	static napi_value CreateMuxer(napi_env env, napi_callback_info info);
	static napi_value GetSupportedFormats(napi_env env, napi_callback_info info);
	static napi_value SetOutput(napi_env env, napi_callback_info info);
	static napi_value Setlatitude(napi_env env, napi_callback_info info);
	static napi_value SetLongitude(napi_env env, napi_callback_info info);
	static napi_value SetOrientationHint(napi_env env, napi_callback_info info);
	static napi_value AddTrack(napi_env env, napi_callback_info info);
	static napi_value Start(napi_env env, napi_callback_info info);
	static napi_value WriteTrackSample(napi_env env, napi_callback_info info);
	static napi_value Stop(napi_env env, napi_callback_info info);
	static napi_value Release(napi_env env, napi_callback_info info);

	MuxerNapi();
	~MuxerNapi();
	
	static napi_ref constructor_;
	napi_env env_ = nullptr;
	napi_ref wrapper_ = nullptr;
	std::shared_ptr<Muxer> muxerImpl_ = nullptr;
};
}  // Media
}  // OHOS
#endif /* MUXER_NAPI_H */
