#include "muxer_napi.h"
#include "media_errors.h"
#include "media_log.h"
#include "common_napi.h"

namespace {
	constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MuxerNapi"};
}

namespace OHOS {
namespace Media {
napi_ref MuxerNapi::constructor_ = nullptr;
const std::string CLASS_NAME = "Muxer";
const std::string PROPERTY_KEY_SIZE = "size";
const std::string PROPERTY_KEY_OFFSET = "offset";
const std::string PROPERTY_KEY_TYPE = "type";
const std::string PROPERTY_KEY_FLAG = "flag";
const std::string PROPERTY_KEY_TIMEUS = "timeUs";
const std::string PROPERTY_KEY_TRACK_ID = "trackId";

struct MuxerNapiAsyncContext {
	napi_env env_;
	napi_async_work work_;
	napi_deferred deferred_;
	napi_ref callbackRef_ = nullptr;
	MuxerNapi* objectInfo_ = nullptr;
	std::string path_;
	std::string format_;
	int32_t setOutputFlag_;
	MediaDescription trackDesc_;
	int32_t trackId_;
	int32_t isAdd_;
	int32_t isStart_;
	void* arrayBuffer_ = nullptr;
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
        char* buffer = (char*)malloc((bufLength + 1) * sizeof(char));
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

MuxerNapi::MuxerNapi()
{
	MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MuxerNapi::~MuxerNapi()
{
	if (wrapper_ != nullptr) {
		napi_delete_reference(env_, wrapper_);
	}
	muxerImpl_ = nullptr;
	MEDIA_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

napi_value MuxerNapi::Init(napi_env env, napi_value exports)
{
	napi_property_descriptor properties[] = {
		DECLARE_NAPI_FUNCTION("setOutput", SetOutput),
		DECLARE_NAPI_FUNCTION("addTrack", AddTrack),
		DECLARE_NAPI_FUNCTION("start", Start),
		DECLARE_NAPI_FUNCTION("writeTrackSample", WriteTrackSample),
		DECLARE_NAPI_FUNCTION("stop", Stop),
		DECLARE_NAPI_FUNCTION("release", Release),

		DECLARE_NAPI_SETTER("latitude", Setlatitude),
		DECLARE_NAPI_SETTER("longitude", SetLongitude),
		DECLARE_NAPI_SETTER("degrees", SetOrientationHint),
	};
	
	napi_property_descriptor staticProperties[] = {
		DECLARE_NAPI_STATIC_FUNCTION("getSupportedFormats", GetSupportedFormats),
		DECLARE_NAPI_STATIC_FUNCTION("createMuxer", CreateMuxer)
	};
	
	napi_value constructor = nullptr;
	napi_status status = napi_define_class(env, CLASS_NAME.c_str(), NAPI_AUTO_LENGTH, MuxerNapi::Constructor,
		nullptr, sizeof(properties) / sizeof(properties[0]), properties, &constructor);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define muxer class");
	
	status = napi_create_reference(env, constructor, 1, &constructor_);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to create reference of constructor");
	
	status = napi_set_named_property(env, exports, CLASS_NAME.c_str(), constructor);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set named property");
	
	status = napi_define_properties(env, exports, sizeof(staticProperties) / sizeof(staticProperties[0]), staticProperties);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to define static properties");
	
	MEDIA_LOGD("Init success");
	return exports;
}

napi_value MuxerNapi::Constructor(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	size_t argCount = 0;
	
	napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result,
		"Failed to retrieve details about the callback");
	
	MuxerNapi* muxerNapi = new(std::nothrow) MuxerNapi();
	CHECK_AND_RETURN_RET_LOG(muxerNapi != nullptr, result, "Failed to create muxerNapi");
	
	muxerNapi->env_ = env;
	muxerNapi->muxerImpl_ = MuxerFactory::CreateMuxer();
	if (muxerNapi->muxerImpl_ == nullptr) {
		delete muxerNapi;
		MEDIA_LOGE("Failed to create muxerImpl");
		return result;
	}
	
	status = napi_wrap(env, jsThis, reinterpret_cast<void*>(muxerNapi),
		MuxerNapi::Destructor, nullptr, &(muxerNapi->wrapper_));
	if (status != napi_ok) {
		delete muxerNapi;
		MEDIA_LOGE("Failed to wrap native instance");
		return result;
	}
	
	MEDIA_LOGD("Constructor success");
	return jsThis;
}

void MuxerNapi::Destructor(napi_env env, void* nativeObject, void* finalize)
{
	(void)env;
	(void)finalize;
	if (nativeObject != nullptr) {
		delete reinterpret_cast<MuxerNapi*>(nativeObject);
	}
	MEDIA_LOGD("Destructor success");
}

napi_value MuxerNapi::CreateMuxer(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value constructor = nullptr;
	napi_status status = napi_get_reference_value(env, constructor_, &constructor);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "Failed to get reference of constructor");
	
	status = napi_new_instance(env, constructor, 0, nullptr, &result);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "Failed to create new instance");
	
	MEDIA_LOGD("Create muxer success");
	return result;
}

napi_value MuxerNapi::GetSupportedFormats(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	MuxerNapi* muxer = nullptr;
	size_t argCount = 0;
	
	napi_status status = napi_get_cb_info(env, info, &argCount, nullptr, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr, result, "Failed to retrieve details about the callbacke");
	
	status = napi_unwrap(env, jsThis, reinterpret_cast<void**>(&muxer));
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && muxer != nullptr, result, "Failed to retrieve instance");
	
	// std::vector<std::string> formats = muxer->muxerImpl_->GetSupportedFormats();
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

static void SetOutputAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
	MuxerNapiAsyncContext* asyncContext = static_cast<MuxerNapiAsyncContext*>(data);
	CHECK_AND_RETURN_LOG(asyncContext != nullptr, "Failed to get MuxerNapiAsyncContext instance");

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
		napi_call_function(env, nullptr, callback, 2, result, &retVal);
		napi_delete_reference(env, asyncContext->callbackRef_);
	}
	napi_delete_async_work(env, asyncContext->work_);
	delete asyncContext;
}

napi_value MuxerNapi::SetOutput(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	napi_value args[3] = {nullptr, nullptr, nullptr};
	size_t argCount = 3;

	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr &&
		args[1] != nullptr && args[2] == nullptr, result, "Failed to retrieve details about the callback");
	
	std::unique_ptr<MuxerNapiAsyncContext> asyncContext = std::make_unique<MuxerNapiAsyncContext>();
	CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create MuxerNapiAsyncContext instance");

	status = napi_unwrap(env, jsThis, reinterpret_cast<void**>(&asyncContext->objectInfo_));
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
		[](napi_env env, void* data) {
			MuxerNapiAsyncContext* context = static_cast<MuxerNapiAsyncContext*>(data);
			context->setOutputFlag_ = context->objectInfo_->muxerImpl_->SetOutput(context->path_, context->format_);
		},
		SetOutputAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work_);
	if (status != napi_ok) {
		MEDIA_LOGE("Failed to create async work");
		napi_get_undefined(env, &result);
	} else {
		napi_queue_async_work(env, asyncContext->work_);
		asyncContext.release();
	}
	return result;
}

napi_value MuxerNapi::Setlatitude(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	MuxerNapi* muxer = nullptr;
	napi_value args[1] = {nullptr};
	size_t argCount = 1;
	
	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr,
		result, "Failed to retrieve details about the callbacke");
	
	status = napi_unwrap(env, jsThis, reinterpret_cast<void**>(&muxer));
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && muxer != nullptr, result, "Failed to retrieve instance");
	
	napi_valuetype valueType = napi_undefined;
	status = napi_typeof(env, args[0], &valueType);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && valueType == napi_number, result,
		"Failed to check argument type");
	
	double latitude;
	status = napi_get_value_double(env, args[0], &latitude);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "Failed to get latitude");
	
	int ret = muxer->muxerImpl_->SetLocation(latitude, 0);
	CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, result, "Failed to call SetLocation");

	MEDIA_LOGD("SetLocation success");
	return result;
}

napi_value MuxerNapi::SetLongitude(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	MuxerNapi* muxer = nullptr;
	napi_value args[1] = {nullptr};
	size_t argCount = 1;
	
	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr,
		result, "Failed to retrieve details about the callbacke");
	
	status = napi_unwrap(env, jsThis, reinterpret_cast<void**>(&muxer));
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && muxer != nullptr, result, "Failed to retrieve instance");
	
	napi_valuetype valueType = napi_undefined;
	status = napi_typeof(env, args[0], &valueType);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && valueType == napi_number, result,
		"Failed to check argument type");
	
	double longitude;
	status = napi_get_value_double(env, args[0], &longitude);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "Failed to get longitude");
	
	int ret = muxer->muxerImpl_->SetLocation(0, longitude);
	CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, result, "Failed to call SetLocation");

	MEDIA_LOGD("SetLocation success");
	return result;
}

napi_value MuxerNapi::SetOrientationHint(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	MuxerNapi* muxer = nullptr;
	napi_value args[1] = {nullptr};
	size_t argCount = 1;
	
	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr,
		result, "Failed to retrieve details about the callbacke");
	
	status = napi_unwrap(env, jsThis, reinterpret_cast<void**>(&muxer));
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && muxer != nullptr, result, "Failed to retrieve instance");
	
	napi_valuetype valueType = napi_undefined;
	status = napi_typeof(env, args[0], &valueType);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && valueType == napi_number, result,
		"Failed to check argument type");
	
	int32_t degrees;
	status = napi_get_value_int32(env, args[0], &degrees);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok, result, "Failed to get degrees");
	
	int32_t ret = muxer->muxerImpl_->SetOrientationHint(degrees);
	CHECK_AND_RETURN_RET_LOG(ret == MSERR_OK, result, "Failed to call SetOrientationHint");

	MEDIA_LOGD("SetOrientationHint success");
	return result;
}

static void AddTrackAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
	MuxerNapiAsyncContext* asyncContext = static_cast<MuxerNapiAsyncContext*>(data);
	CHECK_AND_RETURN_LOG(asyncContext != nullptr, "Failed to get MuxerNapiAsyncContext instance");

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

napi_value MuxerNapi::AddTrack(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	napi_value args[2] = {nullptr, nullptr};
	size_t argCount = 2;

	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr &&
		args[1] != nullptr, result, "Failed to retrieve details about the callback");

	std::unique_ptr<MuxerNapiAsyncContext> asyncContext = std::make_unique<MuxerNapiAsyncContext>();
	CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create MuxerNapiAsyncContext instance");
	
	status = napi_unwrap(env, jsThis, reinterpret_cast<void**>(&asyncContext->objectInfo_));
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->objectInfo_ != nullptr, result, "Failed to retrieve instance");
	
	for (size_t i = 0; i < argCount; ++i) {
		napi_valuetype valueType = napi_undefined;
		napi_typeof(env, args[i], &valueType);
		if (i == 0 && valueType == napi_object) {
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_TRACK_INDEX), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_TRACK_INDEX)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_TRACK_TYPE), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_TRACK_TYPE)));
			// asyncContext->trackDesc_.PutStringValue(std::string(MD_KEY_CODEC_MIME), GetNamedPropertystring(env, args[i], std::string(MD_KEY_CODEC_MIME)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_DURATION), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_DURATION)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_BITRATE), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_BITRATE)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_MAX_INPUT_SIZE), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_MAX_INPUT_SIZE)));
			asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_WIDTH), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_WIDTH)));
			asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_HEIGHT), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_HEIGHT)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_PIXEL_FORMAT), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_PIXEL_FORMAT)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_FRAME_RATE), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_FRAME_RATE)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_CAPTURE_RATE), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_CAPTURE_RATE)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_I_FRAME_INTERVAL), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_I_FRAME_INTERVAL)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_REQUEST_I_FRAME), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_REQUEST_I_FRAME)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_CHANNEL_COUNT), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_CHANNEL_COUNT)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_SAMPLE_RATE), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_SAMPLE_RATE)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_TRACK_COUNT), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_TRACK_COUNT)));
			// asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_CUSTOM_PREFIX), GetNamedPropertyInt32(env, args[i], std::string(MD_KEY_CUSTOM_PREFIX)));
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
		[](napi_env env, void* data) {
			MuxerNapiAsyncContext* context = static_cast<MuxerNapiAsyncContext*>(data);
			context->isAdd_ = context->objectInfo_->muxerImpl_->AddTrack(context->trackDesc_, context->trackId_);
		},
		AddTrackAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work_);
	if (status != napi_ok) {
		MEDIA_LOGE("Failed to create async work");
		napi_get_undefined(env, &result);
	} else {
		napi_queue_async_work(env, asyncContext->work_);
		asyncContext.release();
	}
	return result;
}

static void StartAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
	MuxerNapiAsyncContext* asyncContext = static_cast<MuxerNapiAsyncContext*>(data);
	CHECK_AND_RETURN_LOG(asyncContext != nullptr, "Failed to get MuxerNapiAsyncContext instance");

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
		napi_call_function(env, nullptr, callback, 2, result, &retVal);
		napi_delete_reference(env, asyncContext->callbackRef_);
	}
	napi_delete_async_work(env, asyncContext->work_);
	delete asyncContext;
}

napi_value MuxerNapi::Start(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	napi_value args[1] = {nullptr};
	size_t argCount = 1;

	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr,
		result, "Failed to retrieve details about the callback");

	std::unique_ptr<MuxerNapiAsyncContext> asyncContext = std::make_unique<MuxerNapiAsyncContext>();
	CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create MuxerNapiAsyncContext instance");
	
	status = napi_unwrap(env, jsThis, reinterpret_cast<void**>(&asyncContext->objectInfo_));
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
		[](napi_env env, void* data) {
			MuxerNapiAsyncContext* context = static_cast<MuxerNapiAsyncContext*>(data);
			context->isStart_ = context->objectInfo_->muxerImpl_->Start();
		},
		StartAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work_);
	if (status != napi_ok) {
		MEDIA_LOGE("Failed to create async work");
		napi_get_undefined(env, &result);
	} else {
		napi_queue_async_work(env, asyncContext->work_);
		asyncContext.release();
	}
	return result;
}

static void WriteTrackSampleAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
	MuxerNapiAsyncContext* asyncContext = static_cast<MuxerNapiAsyncContext*>(data);
	CHECK_AND_RETURN_LOG(asyncContext != nullptr, "Failed to get MuxerNapiAsyncContext instance");

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
		napi_call_function(env, nullptr, callback, 2, result, &retVal);
		napi_delete_reference(env, asyncContext->callbackRef_);
	}
	napi_delete_async_work(env, asyncContext->work_);
	delete asyncContext;
}

napi_value MuxerNapi::WriteTrackSample(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	napi_value args[3] = {nullptr, nullptr, nullptr};
	size_t argCount = 3;

	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr && args[1] != nullptr &&
		args[2] != nullptr, result, "Failed to retrieve details about the callback");
	
	std::unique_ptr<MuxerNapiAsyncContext> asyncContext = std::make_unique<MuxerNapiAsyncContext>();
	CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create MuxerNapiAsyncContext instance");

	status = napi_unwrap(env, jsThis, reinterpret_cast<void**>(&asyncContext->objectInfo_));
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->objectInfo_ != nullptr, result, "Failed to retrieve instance");
	
	for (size_t i = 0; i < argCount; ++i) {
		napi_valuetype valueType = napi_undefined;
		if (i == 0) {
			bool isArrayBuffer;
			napi_is_arraybuffer(env, args[i], &isArrayBuffer);
			if (isArrayBuffer) {
				napi_get_arraybuffer_info(env, args[i], &(asyncContext->arrayBuffer_), &(asyncContext->arrayBufferSize_));
			} else {
				MEDIA_LOGE("Failed to check argument value type");
				return result;
			}
		} else {
			napi_typeof(env, args[i], &valueType);
			if (i == 1 && valueType == napi_object) {
				asyncContext->trackSampleInfo_.size = GetNamedPropertyInt32(env, args[i], PROPERTY_KEY_SIZE);
				asyncContext->trackSampleInfo_.offset = GetNamedPropertyInt32(env, args[i], PROPERTY_KEY_OFFSET);
				asyncContext->trackSampleInfo_.flags = static_cast<FrameFlags>(GetNamedPropertyInt32(env, args[i], PROPERTY_KEY_FLAG));
				asyncContext->trackSampleInfo_.timeUs = GetNamedPropertyInt64(env, args[i], PROPERTY_KEY_TIMEUS);
				asyncContext->trackSampleInfo_.trackIdx = GetNamedPropertyInt32(env, args[i], PROPERTY_KEY_TRACK_ID);
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
	
	status = napi_create_async_work(env, nullptr, resource,
		[](napi_env env, void* data) {
			MuxerNapiAsyncContext* context = static_cast<MuxerNapiAsyncContext*>(data);
			std::shared_ptr<AVMemory> avMem = std::make_shared<AVMemory>(static_cast<uint8_t*>(context->arrayBuffer_), context->arrayBufferSize_);
			avMem->SetRange(context->trackSampleInfo_.offset, context->trackSampleInfo_.size);
			context->writeSampleFlag_ = context->objectInfo_->muxerImpl_->WriteTrackSample(avMem, context->trackSampleInfo_);
		},
		WriteTrackSampleAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work_);
	if (status != napi_ok) {
		MEDIA_LOGE("Failed to create async work");
		napi_get_undefined(env, &result);
	} else {
		napi_queue_async_work(env, asyncContext->work_);
		asyncContext.release();
	}
	return result;
}

static void StopSampleAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
	MuxerNapiAsyncContext* asyncContext = static_cast<MuxerNapiAsyncContext*>(data);
	CHECK_AND_RETURN_LOG(asyncContext != nullptr, "Failed to get MuxerNapiAsyncContext instance");

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
		napi_call_function(env, nullptr, callback, 2, result, &retVal);
		napi_delete_reference(env, asyncContext->callbackRef_);
	}
	napi_delete_async_work(env, asyncContext->work_);
	delete asyncContext;
}

napi_value MuxerNapi::Stop(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	napi_value args[1] = {nullptr};
	size_t argCount = 1;

	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr,
		result, "Failed to retrieve details about the callback");
	
	std::unique_ptr<MuxerNapiAsyncContext> asyncContext = std::make_unique<MuxerNapiAsyncContext>();
	CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create MuxerNapiAsyncContext instance");

	status = napi_unwrap(env, jsThis, reinterpret_cast<void**>(&asyncContext->objectInfo_));
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
		[](napi_env env, void* data) {
			MuxerNapiAsyncContext* context = static_cast<MuxerNapiAsyncContext*>(data);
			context->isStop_ = context->objectInfo_->muxerImpl_->Stop();
		},
		StopSampleAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work_);
	if (status != napi_ok) {
		MEDIA_LOGE("Failed to create async work");
		napi_get_undefined(env, &result);
	} else {
		napi_queue_async_work(env, asyncContext->work_);
		asyncContext.release();
	}
	return result;
}

static void ReleaseAsyncCallbackComplete(napi_env env, napi_status status, void* data)
{
	MuxerNapiAsyncContext* asyncContext = static_cast<MuxerNapiAsyncContext*>(data);
	CHECK_AND_RETURN_LOG(asyncContext != nullptr, "Failed to get MuxerNapiAsyncContext instance");
	napi_value result[2] = {nullptr, nullptr};
	napi_value retVal;
	napi_get_undefined(env, &result[0]);
	napi_get_undefined(env, &result[1]);

	if (asyncContext->deferred_) {
		napi_resolve_deferred(env, asyncContext->deferred_, result[1]);
	} else {
		napi_value callback = nullptr;
		napi_get_reference_value(env, asyncContext->callbackRef_, &callback);
		napi_call_function(env, nullptr, callback, 2, result, &retVal);
		napi_delete_reference(env, asyncContext->callbackRef_);
	}
	napi_delete_async_work(env, asyncContext->work_);
	delete asyncContext;
}

napi_value MuxerNapi::Release(napi_env env, napi_callback_info info)
{
	napi_value result = nullptr;
	napi_get_undefined(env, &result);
	napi_value jsThis = nullptr;
	napi_value args[1] = {nullptr};
	size_t argCount = 1;

	napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
	CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr && args[0] != nullptr,
		result, "Failed to retrieve details about the callback");
	
	std::unique_ptr<MuxerNapiAsyncContext> asyncContext = std::make_unique<MuxerNapiAsyncContext>();
	CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create MuxerNapiAsyncContext instance");
	
	status = napi_unwrap(env, jsThis, reinterpret_cast<void**>(&asyncContext->objectInfo_));
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
		[](napi_env env, void* data) {
			MuxerNapiAsyncContext* context = static_cast<MuxerNapiAsyncContext*>(data);
			context->objectInfo_->muxerImpl_->Release();
		},
		ReleaseAsyncCallbackComplete, static_cast<void*>(asyncContext.get()), &asyncContext->work_);
	if (status != napi_ok) {
		MEDIA_LOGE("Failed to create async work");
		napi_get_undefined(env, &result);
	} else {
		napi_queue_async_work(env, asyncContext->work_);
		asyncContext.release();
	}
	return result;
}

}  // namespace Media
}  // namespace OHOS
