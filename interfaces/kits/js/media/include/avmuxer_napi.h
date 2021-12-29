#ifndef AVMUXER_NAPI_H
#define AVMUXER_NAPI_H

#include "avmuxer.h"
#include "media_errors.h"
#include "napi/native_api.h"

namespace OHOS {
namespace Media {
class AVMuxerNapi {
public:
	static napi_value Init(napi_env env, napi_value exports);
private:
	static napi_value Constructor(napi_env env, napi_callback_info info);
	static void Destructor(napi_env env, void* nativeObject, void* finalize);
	static napi_value CreateAVMuxer(napi_env env, napi_callback_info info);
	static napi_value GetSupportedFormats(napi_env env, napi_callback_info info);
	static void AsyncSetOutput(napi_env env, void *data);
	static napi_value SetOutput(napi_env env, napi_callback_info info);
	static napi_value SetLocation(napi_env env, napi_callback_info info);
	static napi_value SetOrientationHint(napi_env env, napi_callback_info info);
	static void AsyncAddTrack(napi_env env, void *data);
	static napi_value AddTrack(napi_env env, napi_callback_info info);
	static void AsyncStart(napi_env env, void *data);
	static napi_value Start(napi_env env, napi_callback_info info);
	static void AsyncWriteTrackSample(napi_env env, void *data);
	static napi_value WriteTrackSample(napi_env env, napi_callback_info info);
	static void AsyncStop(napi_env env, void *data);
	static napi_value Stop(napi_env env, napi_callback_info info);
	static void AsyncRelease(napi_env env, void *data);
	static napi_value Release(napi_env env, napi_callback_info info);

	AVMuxerNapi();
	~AVMuxerNapi();
	
	static napi_ref constructor_;
	napi_env env_ = nullptr;
	napi_ref wrapper_ = nullptr;
	std::shared_ptr<AVMuxer> avmuxerImpl_ = nullptr;
};
}  // Media
}  // OHOS
#endif /* AVMUXER_NAPI_H */
