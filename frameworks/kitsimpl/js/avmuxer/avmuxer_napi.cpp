#include "avmuxer_napi.h"
#include "media_errors.h"
#include "media_log.h"
#include "common_napi.h"

namespace {
	constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AVMuxerNapi"};
}

namespace OHOS {
namespace Media {
napi_ref AVMuxerNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "AVMuxer";
const std::string PROPERTY_KEY_SAMPLEINFO = "sampleInfo";
const std::string PROPERTY_KEY_SIZE = "size";
const std::string PROPERTY_KEY_OFFSET = "offset";
const std::string PROPERTY_KEY_TYPE = "type";
const std::string PROPERTY_KEY_FLAG = "flags";
const std::string PROPERTY_KEY_TIMEUS = "timeMs";
const std::string PROPERTY_KEY_TRACK_ID = "trackIndex";
static int32_t num = 0;

struct AVMuxerNapiAsyncContext {
	napi_env env_;
	napi_async_work work_;
	napi_deferred deferred_;
	napi_ref callbackRef_ = nullptr;
	AVMuxerNapi *objectInfo_ = nullptr;
	napi_value instance_;
	std::string path_;
	std::string format_;
	int32_t setOutputFlag_;
	MediaDescription trackDesc_;
	int32_t trackId_;
	int32_t isAdd_;
	int32_t isStart_;
	// napi_ref sample_;
	void *arrayBuffer_ = nullptr;
	size_t arrayBufferSize_;
	int32_t writeSampleFlag_;
	TrackSampleInfo trackSampleInfo_;
	int32_t isStop_;
};

static std::string GetStringArgument(napi_env env, napi_value value)
{
    std::string strValue = "";
    size_t bufLength = 0;
    napi_status status = napi_get_value_string_utf8(env, value, nullptr, 0, &bufLength);
    if (status == napi_ok && bufLength > 0 && bufLength < PATH_MAX) {
        char *buffer = (char *)malloc((bufLength + 1) * sizeof(char));
		CHECK_AND_RETURN_RET_LOG(buffer != nullptr, strValue, "Failed to create buffer");

        status = napi_get_value_string_utf8(env, value, buffer, bufLength + 1, &bufLength);
        if (status == napi_ok) {
			MEDIA_LOGD("Get Success");
            strValue = buffer;
        }
        free(buffer);
        buffer = nullptr;
    }
    return strValue;
}

static int GetNamedPropertyInt32(napi_env env, napi_value obj, const std::string& keyStr)
{
	napi_value value;
	napi_get_named_property(env, obj, keyStr.c_str(), &value);
	int ret;
	napi_get_value_int32(env, value, &ret);
	return ret;
}

static int64_t GetNamedPropertyInt64(napi_env env, napi_value obj, const std::string& keyStr)
{
	napi_value value;
	napi_get_named_property(env, obj, keyStr.c_str(), &value);
	int64_t ret;
	napi_get_value_int64(env, value, &ret);
	return ret;
}

static double GetNamedPropertydouble(napi_env env, napi_value obj, const std::string& keyStr)
{
	napi_value value;
	napi_get_named_property(env, obj, keyStr.c_str(), &value);
	double ret;
	napi_get_value_double(env, value, &ret);
	return ret;
}

static std::string GetNamedPropertystring(napi_env env, napi_value obj, const std::string& keyStr)
{
	napi_value value;
	napi_get_named_property(env, obj, keyStr.c_str(), &value);
	std::string ret;
	ret = GetStringArgument(env, value);
	return ret;
}

AVMuxerNapi::AVMuxerNapi()
{
	MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVMuxerNapi::~AVMuxerNapi()
{
	if (wrapper_ != nullptr) {
		napi_delete_reference(env_, wrapper_);
	}
	avmuxerImpl_ = nullptr;
	MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value AVMuxerNapi::Init(napi_env env, napi_value exports)
{
	napi_property_descriptor properties[] = {
		DECLARE_NAPI_FUNCTION("setOutput", SetOutput),
		DECLARE_NAPI_FUNCTION("addTrack", AddTrack),
		DECLARE_NAPI_FUNCTION("start", Start),
		DECLARE_NAPI_FUNCTION("writeTrackSample", WriteTrackSample),
		DECLARE_NAPI_FUNCTION("stop", Stop),
		DECLARE_NAPI_FUNCTION("release", Release),

		DECLARE_NAPI_SETTER("location", SetLocation),
		DECLARE_NAPI_SETTER("orientationHint", SetOrientationHint),
	};
	
	napi_property_descriptor staticProperties[] = {
		DECLARE_NAPI_STATIC_FUNCTION("getSupportedFormats", GetSupportedFormats),
		DECLARE_NAPI_STATIC_FUNCTION("createAVMuxer", CreateAVMuxer)
	};
	
	napi_value constructor = nullptr;
	napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, Constructor,
		nullptr, sizeof(properties) / sizeof(properties[0]), properties, &constructor);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define avmuxer class");
	
	status = napi_create_reference(env, constructor, 1, &constructor_);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");
	
	status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set named property");
	
	status = napi_define_properties(env, exports, sizeof(staticProperties) / sizeof(staticProperties[0]), staticProperties);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static properties");
	
	MEDIA_LOGD("Init success");
	return exports;
}

napi_value AVMuxerNapi::Constructor(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	size_t argCount = 0;
	
	napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result,
		"Failed to retrieve details about the callback");
	
	AVMuxerNapi *avmuxerNapi = new(std::nothrow) AVMuxerNapi();
	CHECK_AND_RETURN_RET_LOG(avmuxerNapi != nullptr, result, "Failed to create avmuxerNapi");
	
	avmuxerNapi->env_ = env;
	avmuxerNapi->avmuxerImpl_ = AVMuxerFactory::CreateAVMuxer();
	if (avmuxerNapi->avmuxerImpl_ == nullptr) {
		delete avmuxerNapi;
		MEDIA_LOGE("Failed to create avmuxerImpl");
		return result;
	}
	
	status = napi_wrap(env, jsThis, reinterpret_cast<void *>(avmuxerNapi),
		AVMuxerNapi::Destructor, nullptr, &(avmuxerNapi->wrapper_));
	if (status != napi_ok) {
		delete avmuxerNapi;
		MEDIA_LOGE("Failed to wrap native instance");
		return result;
	}
	
	MEDIA_LOGD("Constructor success");
	return jsThis;
}

void AVMuxerNapi::Destructor(napi_env env, void *nativeObject, void *finalize)
{
	(void)env;
	(void)finalize;
	if (nativeObject != nullptr) {
		delete reinterpret_cast<AVMuxerNapi *>(nativeObject);
	}
	MEDIA_LOGD("Destructor success");
}

static void CreateAVMuxerAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
	AVMuxerNapiAsyncContext *asyncContext = static_cast<AVMuxerNapiAsyncContext *>(data);
	CHECK_AND_RETURN_LOG(asyncContext != nullptr, "Failed to get AVMuxerNapiAsyncContext instance");

	napi_value result[2] = {nullptr, nullptr};
	napi_value retVal;
	napi_get_undefined(env, &result[0]);
	napi_get_undefined(env, &result[1]);

	napi_value instance_ = asyncContext->instance_;
	if (instance_ == nullptr) {
		napi_value msgValStr = nullptr;
		napi_create_string_utf8(env, "invalid argument", NAPI_AUTO_LENGTH, &msgValStr);
		napi_create_error(env, nullptr, msgValStr, &result[0]);
		CommonNapi::FillErrorArgs(env, static_cast<int32_t>(MSERR_EXT_INVALID_VAL), result[0]);
	} else {
		result[1] = instance_;
	}

	if (asyncContext->deferred_) {
		if (instance_ == nullptr) {
			napi_reject_deferred(env, asyncContext->deferred_, result[0]);
			MEDIA_LOGD("instance_ == nullptr");
		} else {
			napi_resolve_deferred(env, asyncContext->deferred_, result[1]);
			MEDIA_LOGD("instance_ != nullptr");
		}
	} else {
		napi_value callback = nullptr;
		napi_get_reference_value(env, asyncContext->callbackRef_, &callback);
		napi_call_function(env, nullptr, callback, 2, result, &retVal);
		napi_delete_reference(env, asyncContext->callbackRef_);
	}
	napi_delete_async_work(env, asyncContext->work_);
	delete asyncContext;
}

napi_value AVMuxerNapi::CreateAVMuxer(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	napi_value args[1] = {nullptr};
	size_t argCount = 1;

	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr,
		result, "Failed to retrieve details about the callback");

	std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>();
	CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

	for (size_t i = 0; i < argCount; ++i) {
		napi_valuetype valueType = napi_undefined;
		napi_typeof(env, args[i], &valueType);
		if (i == 0 && valueType == napi_function) {
			napi_create_reference(env, args[i], 1, &asyncContext->callbackRef_);
		} else {
			MEDIA_LOGE("Failed to check argument value type");
			return result;
		}
	}

	if (asyncContext->callbackRef_ == nullptr) {
		napi_create_promise(env, &asyncContext->deferred_, &result);
	}

	napi_value resource = nullptr;
	napi_create_string_utf8(env, "CreateAVMuxer", NAPI_AUTO_LENGTH, &resource);

	status = napi_create_async_work(env, nullptr, resource,
		[](napi_env env, void *data) {
			AVMuxerNapiAsyncContext *context = static_cast<AVMuxerNapiAsyncContext *>(data);
			CHECK_AND_RETURN_LOG(context != nullptr, "AVMuxerNapiAsyncContext is nullptr!");
			
			napi_value constructor = nullptr;
			napi_status status = napi_get_reference_value(env, constructor_, &constructor);
			CHECK_AND_RETURN_LOG(status == napi_ok, "Failed to get reference of constructor");
	
			status = napi_new_instance(env, constructor, 0, nullptr, &(context->instance_));
			CHECK_AND_RETURN_LOG(status == napi_ok, "Failed to create new instance");
	
			MEDIA_LOGD("Create avmuxer success");
		},
		CreateAVMuxerAsyncCallbackComplete, static_cast<void *>(asyncContext.get()), &asyncContext->work_);
	if (status != napi_ok) {
		MEDIA_LOGE("Failed to create async work");
		napi_get_undefined(env, &result);
	} else {
		napi_queue_async_work(env, asyncContext->work_);
		asyncContext.release();
	}
	return result;
}

napi_value AVMuxerNapi::GetSupportedFormats(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	AVMuxerNapi *avmuxer = nullptr;
	size_t argCount = 0;
	
	napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result, "Failed to retrieve details about the callbacke");
	
	status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&avmuxer));
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && avmuxer != nullptr, result, "Failed to retrieve instance");
	
	// std::vector<std::string> formats = avmuxer->avmuxerImpl_->GetSupportedFormats();
	std::vector<std::string> formats = {};
	size_t size = formats.size();
	napi_create_array_with_length(env, size, &result);
	for (unsigned int i = 0; i < size; ++i) {
		std::string format = formats[i];
		napi_value value = nullptr;
		status = napi_create_string_utf8(env, format.c_str(), NAPI_AUTO_LENGTH, &value);
		if (status != napi_ok) {
			continue;
		}
		napi_set_element(env, result, i, value);
	}
	return result;
}

static void SetOutputAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
	AVMuxerNapiAsyncContext *asyncContext = static_cast<AVMuxerNapiAsyncContext *>(data);
	CHECK_AND_RETURN_LOG(asyncContext != nullptr, "Failed to get AVMuxerNapiAsyncContext instance");

	napi_value result[2] = {nullptr, nullptr};
	napi_value retVal;
	napi_get_undefined(env, &result[0]);
	napi_get_undefined(env, &result[1]);

	int setOutputFlag = asyncContext->setOutputFlag_;
	if (setOutputFlag != MSERR_OK) {
		napi_value msgValStr = nullptr;
		napi_create_string_utf8(env, "invalid argument", NAPI_AUTO_LENGTH, &msgValStr);
		napi_create_error(env, nullptr, msgValStr, &result[0]);
		CommonNapi::FillErrorArgs(env, static_cast<int32_t>(MSERR_EXT_INVALID_VAL), result[0]);
	}

	if (asyncContext->deferred_) {
		if (setOutputFlag != MSERR_OK) {
			napi_reject_deferred(env, asyncContext->deferred_, result[0]);
		} else {
			napi_resolve_deferred(env, asyncContext->deferred_, result[1]);
		}
	} else {
		napi_value callback = nullptr;
		napi_get_reference_value(env, asyncContext->callbackRef_, &callback);
		napi_call_function(env, nullptr, callback, 1, &result[0], &retVal);
		napi_delete_reference(env, asyncContext->callbackRef_);
	}
	napi_delete_async_work(env, asyncContext->work_);
	delete asyncContext;
}

napi_value AVMuxerNapi::SetOutput(napi_env env, napi_callback_info info)
{
	MEDIA_LOGD("begin SetOutput");
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	napi_value args[3] = {nullptr, nullptr, nullptr};
	size_t argCount = 3;

	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr,
		result, "Failed to retrieve details about the callback");
	
	std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>();
	CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

	status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->objectInfo_));
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->objectInfo_ != nullptr, result, "Failed to retrieve instance");
	
	for (size_t i = 0; i < argCount; ++i) {
		napi_valuetype valueType = napi_undefined;
		napi_typeof(env, args[i], &valueType);
		if (i == 0 && valueType == napi_string) {
			asyncContext->path_ = GetStringArgument(env, args[i]);
		} else if (i == 1 && valueType == napi_string) {
			asyncContext->format_ = GetStringArgument(env, args[i]);
		} else if (i == 2 && valueType == napi_function) {
			napi_create_reference(env, args[i], 1, &asyncContext->callbackRef_);
		} else {
			MEDIA_LOGE("Failed to check argument value type");
			return result;
		}
	}
	
	if (asyncContext->callbackRef_ == nullptr) {
		napi_create_promise(env, &asyncContext->deferred_, &result);
	}
	
	napi_value resource = nullptr;
	napi_create_string_utf8(env, "SetOutput", NAPI_AUTO_LENGTH, &resource);
	
	status = napi_create_async_work(env, nullptr, resource,
		[](napi_env env, void *data) {
			AVMuxerNapiAsyncContext *context = static_cast<AVMuxerNapiAsyncContext *>(data);
			context->setOutputFlag_ = context->objectInfo_->avmuxerImpl_->SetOutput(context->path_, context->format_);
		},
		SetOutputAsyncCallbackComplete, static_cast<void *>(asyncContext.get()), &asyncContext->work_);
	if (status != napi_ok) {
		MEDIA_LOGE("Failed to create async work");
		napi_get_undefined(env, &result);
	} else {
		napi_queue_async_work(env, asyncContext->work_);
		asyncContext.release();
	}

	MEDIA_LOGD("SetOutput success");
	return result;
}


napi_value AVMuxerNapi::SetLocation(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	AVMuxerNapi *avmuxer = nullptr;
	napi_value args[1] = {nullptr};
	size_t argCount = 1;
	
	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr,
		result, "Failed to retrieve details about the callbacke");
	
	status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&avmuxer));
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && avmuxer != nullptr, result, "Failed to retrieve instance");
	
	napi_valuetype valueType = napi_undefined;
	status = napi_typeof(env, args[0], &valueType);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && valueType == napi_object, result,
		"Failed to check argument type");
	
	double latitude;
	double longitude;
	latitude = GetNamedPropertydouble(env, args[0], "latitude");
	longitude = GetNamedPropertydouble(env, args[0], "longitude");
	
	int ret = avmuxer->avmuxerImpl_->SetLocation(latitude, longitude);
	CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, result, "Failed to call SetLocation");

	MEDIA_LOGD("SetLocation success");
	return result;
}

napi_value AVMuxerNapi::SetOrientationHint(napi_env env, napi_callback_info info)
{
	MEDIA_LOGD("SetOrientationHint begin");
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	AVMuxerNapi *avmuxer = nullptr;
	napi_value args[1] = {nullptr};
	size_t argCount = 1;
	
	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr,
		result, "Failed to retrieve details about the callbacke");
	
	status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&avmuxer));
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && avmuxer != nullptr, result, "Failed to retrieve instance");
	
	napi_valuetype valueType = napi_undefined;
	status = napi_typeof(env, args[0], &valueType);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && valueType == napi_number, result,
		"Failed to check argument type");
	
	int32_t degrees;
	status = napi_get_value_int32(env, args[0], &degrees);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "Failed to get degrees");
	
	int32_t ret = avmuxer->avmuxerImpl_->SetOrientationHint(degrees);
	CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, result, "Failed to call SetOrientationHint");

	MEDIA_LOGD("SetOrientationHint success");
	return result;
}

static void AddTrackAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
	AVMuxerNapiAsyncContext *asyncContext = static_cast<AVMuxerNapiAsyncContext *>(data);
	CHECK_AND_RETURN_LOG(asyncContext != nullptr, "Failed to get AVMuxerNapiAsyncContext instance");

	napi_value result[2] = {nullptr, nullptr};
	napi_value retVal;
	napi_get_undefined(env, &result[0]);
	napi_get_undefined(env, &result[1]);

	int isAdd = asyncContext->isAdd_;
	if (isAdd != MSERR_OK) {
		napi_value msgValStr = nullptr;
		napi_create_string_utf8(env, "invalid argument", NAPI_AUTO_LENGTH, &msgValStr);
		napi_create_error(env, nullptr, msgValStr, &result[0]);
		CommonNapi::FillErrorArgs(env, static_cast<int32_t>(MSERR_EXT_INVALID_VAL), result[0]);
	} else {
		MEDIA_LOGD("trackId_ is: %{public}d", asyncContext->trackId_);
		napi_create_int32(env, asyncContext->trackId_, &result[1]);
	}

	if (asyncContext->deferred_) {
		if (isAdd != MSERR_OK) {
			napi_reject_deferred(env, asyncContext->deferred_, result[0]);
		} else {
			napi_resolve_deferred(env, asyncContext->deferred_, result[1]);
		}
	} else {
		napi_value callback = nullptr;
		napi_get_reference_value(env, asyncContext->callbackRef_, &callback);
		napi_call_function(env, nullptr, callback, 2, result, &retVal);
		napi_delete_reference(env, asyncContext->callbackRef_);
	}
	napi_delete_async_work(env, asyncContext->work_);
	delete asyncContext;
}

napi_value AVMuxerNapi::AddTrack(napi_env env, napi_callback_info info)
{
	MEDIA_LOGD("AddTrack begin");
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	napi_value args[2] = {nullptr, nullptr};
	size_t argCount = 2;

	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr,
		result, "Failed to retrieve details about the callback");

	std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>();
	CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");
	
	status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->objectInfo_));
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->objectInfo_ != nullptr, result, "Failed to retrieve instance");
	
	for (size_t i = 0; i < argCount; ++i) {
		napi_valuetype valueType = napi_undefined;
		napi_typeof(env, args[i], &valueType);
		// if (i == 0 && valueType == napi_object) {
		if (i == 0) {
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_TRACK_INDEX), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_TRACK_INDEX)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_TRACK_TYPE), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_TRACK_TYPE)));
			// asyncContext->trackDesc_.PutStringValue(std::string(MD_KEY_CODEC_MIME), GetNamedPropertystring(env, args[i], std::string(MD_KEY_CODEC_MIME)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_DURATION), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_DURATION)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_BITRATE), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_BITRATE)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_MAX_INPUT_SIZE), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_MAX_INPUT_SIZE)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_WIDTH), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_WIDTH)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_HEIGHT), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_HEIGHT)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_PIXEL_FORMAT), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_PIXEL_FORMAT)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_FRAME_RATE), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_FRAME_RATE)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_CAPTURE_RATE), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_CAPTURE_RATE)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_I_FRAME_INTERVAL), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_I_FRAME_INTERVAL)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_REQUEST_I_FRAME), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_REQUEST_I_FRAME)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_CHANNEL_COUNT), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_CHANNEL_COUNT)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_SAMPLE_RATE), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_SAMPLE_RATE)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_TRACK_COUNT), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_TRACK_COUNT)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_CUSTOM_PREFIX), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_CUSTOM_PREFIX)));
			if (num == 0) {
				asyncContext->trackDesc_.PutStringValue(std::string(MD_KEY_CODEC_MIME), "video/x-h264");
			} else {
				asyncContext->trackDesc_.PutStringValue(std::string(MD_KEY_CODEC_MIME), "audio/mpeg");
			}
			asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_WIDTH), 480);
			asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_HEIGHT), 640);
			asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_FRAME_RATE), 30);
			asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_CHANNEL_COUNT), 2);
			asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_SAMPLE_RATE), 441000);
		} else if (i == 1 && valueType == napi_function) {
			napi_create_reference(env, args[i], 1, &asyncContext->callbackRef_);
		} else {
			MEDIA_LOGE("Failed to check argument value type");
			return result;
		}
	}
	
	if (asyncContext->callbackRef_ == nullptr) {
		napi_create_promise(env, &asyncContext->deferred_, &result);
	}
	
	napi_value resource = nullptr;
	napi_create_string_utf8(env, "AddTrack", NAPI_AUTO_LENGTH, &resource);
	
	status = napi_create_async_work(env, nullptr, resource,
		[](napi_env env, void *data) {
			AVMuxerNapiAsyncContext *context = static_cast<AVMuxerNapiAsyncContext *>(data);
			context->isAdd_ = context->objectInfo_->avmuxerImpl_->AddTrack(context->trackDesc_, context->trackId_);
			MEDIA_LOGD("trackId_ is: %{public}d", context->trackId_);
		},
		AddTrackAsyncCallbackComplete, static_cast<void *>(asyncContext.get()), &asyncContext->work_);
	if (status != napi_ok) {
		MEDIA_LOGE("Failed to create async work");
		napi_get_undefined(env, &result);
	} else {
		napi_queue_async_work(env, asyncContext->work_);
		asyncContext.release();
	}
	num++;
	MEDIA_LOGD("AddTrack success");
	return result;
}

static void StartAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
	AVMuxerNapiAsyncContext *asyncContext = static_cast<AVMuxerNapiAsyncContext *>(data);
	CHECK_AND_RETURN_LOG(asyncContext != nullptr, "Failed to get AVMuxerNapiAsyncContext instance");

	napi_value result[2] = {nullptr, nullptr};
	napi_value retVal;
	napi_get_undefined(env, &result[0]);
	napi_get_undefined(env, &result[1]);

	int isStart = asyncContext->isStart_;
	if (isStart != MSERR_OK) {
		napi_value msgValStr = nullptr;
		napi_create_string_utf8(env, "invalid argument", NAPI_AUTO_LENGTH, &msgValStr);
		napi_create_error(env, nullptr, msgValStr, &result[0]);
		CommonNapi::FillErrorArgs(env, static_cast<int32_t>(MSERR_EXT_INVALID_VAL), result[0]);
	}

	if (asyncContext->deferred_) {
		if (isStart != MSERR_OK) {
			napi_reject_deferred(env, asyncContext->deferred_, result[0]);
		} else {
			napi_resolve_deferred(env, asyncContext->deferred_, result[1]);
		}
	} else {
		napi_value callback = nullptr;
		napi_get_reference_value(env, asyncContext->callbackRef_, &callback);
		napi_call_function(env, nullptr, callback, 1, &result[0], &retVal);
		napi_delete_reference(env, asyncContext->callbackRef_);
	}
	napi_delete_async_work(env, asyncContext->work_);
	delete asyncContext;
}

napi_value AVMuxerNapi::Start(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	napi_value args[1] = {nullptr};
	size_t argCount = 1;

	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr,
		result, "Failed to retrieve details about the callback");

	std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>();
	CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");
	
	status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->objectInfo_));
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->objectInfo_ != nullptr, result, "Failed to retrieve instance");
	
	for (size_t i = 0; i < argCount; ++i) {
		napi_valuetype valueType = napi_undefined;
		napi_typeof(env, args[i], &valueType);
		if (i == 0 && valueType == napi_function) {
			napi_create_reference(env, args[i], 1, &asyncContext->callbackRef_);
		} else {
			MEDIA_LOGE("Failed to check argument value type");
			return result;
		}
	}
	
	if (asyncContext->callbackRef_ == nullptr) {
		napi_create_promise(env, &asyncContext->deferred_, &result);
	}
	
	napi_value resource = nullptr;
	napi_create_string_utf8(env, "Start", NAPI_AUTO_LENGTH, &resource);
	
	status = napi_create_async_work(env, nullptr, resource,
		[](napi_env env, void *data) {
			AVMuxerNapiAsyncContext *context = static_cast<AVMuxerNapiAsyncContext *>(data);
			context->isStart_ = context->objectInfo_->avmuxerImpl_->Start();
		},
		StartAsyncCallbackComplete, static_cast<void *>(asyncContext.get()), &asyncContext->work_);
	if (status != napi_ok) {
		MEDIA_LOGE("Failed to create async work");
		napi_get_undefined(env, &result);
	} else {
		napi_queue_async_work(env, asyncContext->work_);
		asyncContext.release();
	}
	return result;
}

static void WriteTrackSampleAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
	AVMuxerNapiAsyncContext *asyncContext = static_cast<AVMuxerNapiAsyncContext *>(data);
	CHECK_AND_RETURN_LOG(asyncContext != nullptr, "Failed to get AVMuxerNapiAsyncContext instance");

	napi_value result[2] = {nullptr, nullptr};
	napi_value retVal;
	napi_get_undefined(env, &result[0]);
	napi_get_undefined(env, &result[1]);

	int writeSampleFlag = asyncContext->writeSampleFlag_;
	if (writeSampleFlag != MSERR_OK) {
		napi_value msgValStr = nullptr;
		napi_create_string_utf8(env, "invalid argument", NAPI_AUTO_LENGTH, &msgValStr);
		napi_create_error(env, nullptr, msgValStr, &result[0]);
		CommonNapi::FillErrorArgs(env, static_cast<int32_t>(MSERR_EXT_INVALID_VAL), result[0]);
	}

	if (asyncContext->deferred_) {
		if (writeSampleFlag != MSERR_OK) {
			napi_reject_deferred(env, asyncContext->deferred_, result[0]);
		} else {
			napi_resolve_deferred(env, asyncContext->deferred_, result[1]);
		}
	} else {
		napi_value callback = nullptr;
		napi_get_reference_value(env, asyncContext->callbackRef_, &callback);
		napi_call_function(env, nullptr, callback, 1, &result[0], &retVal);
		napi_delete_reference(env, asyncContext->callbackRef_);
	}
	napi_delete_async_work(env, asyncContext->work_);
	delete asyncContext;
}

napi_value AVMuxerNapi::WriteTrackSample(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	napi_value args[3] = {nullptr, nullptr, nullptr};
	size_t argCount = 3;

	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr,
		result, "Failed to retrieve details about the callback");
	
	std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>();
	CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

	status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->objectInfo_));
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->objectInfo_ != nullptr, result, "Failed to retrieve instance");
	
	for (size_t i = 0; i < argCount; ++i) {
		MEDIA_LOGD("begin for");
		napi_valuetype valueType = napi_undefined;
		if (i == 0) {
			bool isArrayBuffer;
			napi_is_arraybuffer(env, args[i], &isArrayBuffer);
			if (isArrayBuffer) {
				MEDIA_LOGI("isArrayBuffer");
				napi_get_arraybuffer_info(env, args[i], &(asyncContext->arrayBuffer_), &(asyncContext->arrayBufferSize_));
				// napi_create_reference(env, args[i], 1, &asyncContext->sample_);
				// MEDIA_LOGD("asyncContext->arrayBufferSize_ is: %{public}d", asyncContext->arrayBufferSize_);
				MEDIA_LOGI("data[0] is: %{public}u, data[1] is: %{public}u, data[2] is: %{public}u, data[3] is: %{public}u,", ((uint8_t*)(asyncContext->arrayBuffer_))[0], ((uint8_t*)(asyncContext->arrayBuffer_))[1], ((uint8_t*)(asyncContext->arrayBuffer_))[2], ((uint8_t*)(asyncContext->arrayBuffer_))[3]);
			} else {
				MEDIA_LOGE("Failed to check argument value type");
				return result;
			}
		} else {
			napi_typeof(env, args[i], &valueType);
			if (i == 1 && valueType == napi_object) {
				napi_value trackSampleInfo;
				napi_get_named_property(env, args[i], PROPERTY_KEY_SAMPLEINFO.c_str(), &trackSampleInfo);
				asyncContext->trackSampleInfo_.size = GetNamedPropertyInt32(env, trackSampleInfo, PROPERTY_KEY_SIZE);
				asyncContext->trackSampleInfo_.offset = GetNamedPropertyInt32(env, trackSampleInfo, PROPERTY_KEY_OFFSET);
				asyncContext->trackSampleInfo_.flags = static_cast<FrameFlags>(GetNamedPropertyInt32(env, trackSampleInfo, PROPERTY_KEY_FLAG));
				asyncContext->trackSampleInfo_.timeUs = GetNamedPropertyInt64(env, trackSampleInfo, PROPERTY_KEY_TIMEUS);
				asyncContext->trackSampleInfo_.trackIdx = GetNamedPropertyInt32(env, args[i], PROPERTY_KEY_TRACK_ID);
				MEDIA_LOGD("size is: %{public}d", asyncContext->trackSampleInfo_.size);
				MEDIA_LOGD("offset is: %{public}d", asyncContext->trackSampleInfo_.offset);
				MEDIA_LOGD("flags is: %{public}d", asyncContext->trackSampleInfo_.flags);
				MEDIA_LOGD("timeUs is: %{public}lld", asyncContext->trackSampleInfo_.timeUs);
				MEDIA_LOGD("trackIdx is: %{public}d", asyncContext->trackSampleInfo_.trackIdx);
			} else if (i == 2 && valueType == napi_function) {
				napi_create_reference(env, args[i], 1, &asyncContext->callbackRef_);
			} else {
				MEDIA_LOGE("Failed to check argument value type");
				return result;
			}
		}
	}
	
	if (asyncContext->callbackRef_ == nullptr) {
		napi_create_promise(env, &asyncContext->deferred_, &result);
	}
	
	napi_value resource = nullptr;
	napi_create_string_utf8(env, "WriteTrackSample", NAPI_AUTO_LENGTH, &resource);

	std::shared_ptr<AVMemory> avMem = std::make_shared<AVMemory>(static_cast<uint8_t *>(asyncContext->arrayBuffer_), asyncContext->arrayBufferSize_);
	avMem->SetRange(asyncContext->trackSampleInfo_.offset, asyncContext->trackSampleInfo_.size);
	// MEDIA_LOGD("size is: %{public}d", asyncContext->trackSampleInfo_.size);
	// MEDIA_LOGD("offset is: %{public}d", asyncContext->trackSampleInfo_.offset);
	// MEDIA_LOGD("flags is: %{public}d", asyncContext->trackSampleInfo_.flags);
	// MEDIA_LOGD("timeUs is: %{public}lld", asyncContext->trackSampleInfo_.timeUs);
	// MEDIA_LOGD("trackIdx is: %{public}d", asyncContext->trackSampleInfo_.trackIdx);
	MEDIA_LOGI("data[0] is: %{public}u, data[1] is: %{public}u, data[2] is: %{public}u, data[3] is: %{public}u,", ((uint8_t*)(asyncContext->arrayBuffer_))[0], ((uint8_t*)(asyncContext->arrayBuffer_))[1], ((uint8_t*)(asyncContext->arrayBuffer_))[2], ((uint8_t*)(asyncContext->arrayBuffer_))[3]);
	asyncContext->writeSampleFlag_ = asyncContext->objectInfo_->avmuxerImpl_->WriteTrackSample(avMem, asyncContext->trackSampleInfo_);
	
	status = napi_create_async_work(env, nullptr, resource,
		[](napi_env env, void *data) {
			// AVMuxerNapiAsyncContext* context = static_cast<AVMuxerNapiAsyncContext*>(data);
			// napi_value sampleData = nullptr;
			// MEDIA_LOGD("begin arraybuffer");
			// napi_get_reference_value(env, context->sample_, &sampleData);
			// MEDIA_LOGD("begin arraybuffer1");
			// napi_get_arraybuffer_info(env, sampleData, &(context->arrayBuffer_), &(context->arrayBufferSize_));
			// std::shared_ptr<AVMemory> avMem = std::make_shared<AVMemory>(static_cast<uint8_t*>(context->arrayBuffer_), context->arrayBufferSize_);
			// avMem->SetRange(context->trackSampleInfo_.offset, context->trackSampleInfo_.size);
			// MEDIA_LOGD("size is: %{public}d", context->trackSampleInfo_.size);
			// MEDIA_LOGD("offset is: %{public}d", context->trackSampleInfo_.offset);
			// MEDIA_LOGD("flags is: %{public}d", context->trackSampleInfo_.flags);
			// MEDIA_LOGD("timeUs is: %{public}lld", context->trackSampleInfo_.timeUs);
			// MEDIA_LOGD("trackIdx is: %{public}d", context->trackSampleInfo_.trackIdx);
			// MEDIA_LOGD("data[0] is: %{public}u, data[1] is: %{public}u, data[2] is: %{public}u, data[3] is: %{public}u,", ((uint8_t*)(context->arrayBuffer_))[0], ((uint8_t*)(context->arrayBuffer_))[1], ((uint8_t*)(context->arrayBuffer_))[2], ((uint8_t*)(context->arrayBuffer_))[3]);
			// context->writeSampleFlag_ = context->objectInfo_->avmuxerImpl_->WriteTrackSample(avMem, context->trackSampleInfo_);
			// napi_delete_reference(env, context->sample_);
		},
		WriteTrackSampleAsyncCallbackComplete, static_cast<void *>(asyncContext.get()), &asyncContext->work_);
	if (status != napi_ok) {
		MEDIA_LOGE("Failed to create async work");
		napi_get_undefined(env, &result);
	} else {
		napi_queue_async_work(env, asyncContext->work_);
		asyncContext.release();
	}
	return result;
}

static void StopSampleAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
	AVMuxerNapiAsyncContext *asyncContext = static_cast<AVMuxerNapiAsyncContext *>(data);
	CHECK_AND_RETURN_LOG(asyncContext != nullptr, "Failed to get AVMuxerNapiAsyncContext instance");

	napi_value result[2] = {nullptr, nullptr};
	napi_value retVal;
	napi_get_undefined(env, &result[0]);
	napi_get_undefined(env, &result[1]);

	int stoped = asyncContext->isStop_;
	if (stoped != MSERR_OK) {
		napi_value msgValStr = nullptr;
		napi_create_string_utf8(env, "invalid argument", NAPI_AUTO_LENGTH, &msgValStr);
		napi_create_error(env, nullptr, msgValStr, &result[0]);
		CommonNapi::FillErrorArgs(env, static_cast<int32_t>(MSERR_EXT_INVALID_VAL), result[0]);
	}

	if (asyncContext->deferred_) {
		if (stoped != MSERR_OK) {
			napi_reject_deferred(env, asyncContext->deferred_, result[0]);
		} else {
			napi_resolve_deferred(env, asyncContext->deferred_, result[1]);
		}
	} else {
		napi_value callback = nullptr;
		napi_get_reference_value(env, asyncContext->callbackRef_, &callback);
		napi_call_function(env, nullptr, callback, 1, &result[0], &retVal);
		napi_delete_reference(env, asyncContext->callbackRef_);
	}
	napi_delete_async_work(env, asyncContext->work_);
	delete asyncContext;
}

napi_value AVMuxerNapi::Stop(napi_env env, napi_callback_info info)
{
	MEDIA_LOGD("Stop begin");
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	napi_value args[1] = {nullptr};
	size_t argCount = 1;

	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr,
		result, "Failed to retrieve details about the callback");
	
	std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>();
	CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

	status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->objectInfo_));
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->objectInfo_ != nullptr, result, "Failed to retrieve instance");
	
	for (size_t i = 0; i < argCount; ++i) {
		napi_valuetype valueType = napi_undefined;
		napi_typeof(env, args[i], &valueType);
		if (i == 0 && valueType == napi_function) {
			napi_create_reference(env, args[i], 1, &asyncContext->callbackRef_);
		} else {
			MEDIA_LOGE("Failed to check argument value type");
			return result;
		}
	}
	
	if (asyncContext->callbackRef_ == nullptr) {
		napi_create_promise(env, &asyncContext->deferred_, &result);
	}
	
	napi_value resource = nullptr;
	napi_create_string_utf8(env, "Stop", NAPI_AUTO_LENGTH, &resource);
	
	status = napi_create_async_work(env, nullptr, resource,
		[](napi_env env, void *data) {
			AVMuxerNapiAsyncContext *context = static_cast<AVMuxerNapiAsyncContext *>(data);
			context->isStop_ = context->objectInfo_->avmuxerImpl_->Stop();
		},
		StopSampleAsyncCallbackComplete, static_cast<void *>(asyncContext.get()), &asyncContext->work_);
	if (status != napi_ok) {
		MEDIA_LOGE("Failed to create async work");
		napi_get_undefined(env, &result);
	} else {
		napi_queue_async_work(env, asyncContext->work_);
		asyncContext.release();
	}
	return result;
	MEDIA_LOGD("Stop success");
}

static void ReleaseAsyncCallbackComplete(napi_env env, napi_status status, void *data)
{
	AVMuxerNapiAsyncContext *asyncContext = static_cast<AVMuxerNapiAsyncContext *>(data);
	CHECK_AND_RETURN_LOG(asyncContext != nullptr, "Failed to get AVMuxerNapiAsyncContext instance");
	napi_value result[2] = {nullptr, nullptr};
	napi_value retVal;
	napi_get_undefined(env, &result[0]);
	napi_get_undefined(env, &result[1]);

	if (asyncContext->deferred_) {
		napi_resolve_deferred(env, asyncContext->deferred_, result[1]);
	} else {
		napi_value callback = nullptr;
		napi_get_reference_value(env, asyncContext->callbackRef_, &callback);
		napi_call_function(env, nullptr, callback, 1, &result[0], &retVal);
		napi_delete_reference(env, asyncContext->callbackRef_);
	}
	napi_delete_async_work(env, asyncContext->work_);
	delete asyncContext;
}

napi_value AVMuxerNapi::Release(napi_env env, napi_callback_info info)
{
	MEDIA_LOGD("Release begin");
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	napi_value args[1] = {nullptr};
	size_t argCount = 1;

	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr,
		result, "Failed to retrieve details about the callback");
	
	std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>();
	CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");
	
	status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->objectInfo_));
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->objectInfo_ != nullptr, result, "Failed to retrieve instance");
	
	for (size_t i = 0; i < argCount; ++i) {
		napi_valuetype valueType = napi_undefined;
		napi_typeof(env, args[i], &valueType);
		if (i == 0 && valueType == napi_function) {
			napi_create_reference(env, args[i], 1, &asyncContext->callbackRef_);
		} else {
			MEDIA_LOGE("Failed to check argument value type");
			return result;
		}
	}
	
	if (asyncContext->callbackRef_ == nullptr) {
		napi_create_promise(env, &asyncContext->deferred_, &result);
	}
	
	napi_value resource = nullptr;
	napi_create_string_utf8(env, "Release", NAPI_AUTO_LENGTH, &resource);
	
	status = napi_create_async_work(env, nullptr, resource,
		[](napi_env env, void *data) {
			AVMuxerNapiAsyncContext *context = static_cast<AVMuxerNapiAsyncContext *>(data);
			context->objectInfo_->avmuxerImpl_->Release();
		},
		ReleaseAsyncCallbackComplete, static_cast<void *>(asyncContext.get()), &asyncContext->work_);
	if (status != napi_ok) {
		MEDIA_LOGE("Failed to create async work");
		napi_get_undefined(env, &result);
	} else {
		napi_queue_async_work(env, asyncContext->work_);
		asyncContext.release();
	}
	return result;
	MEDIA_LOGD("Release end");
}

}  // namespace Media
}  // namespace OHOS
