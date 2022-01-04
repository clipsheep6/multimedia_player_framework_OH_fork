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

struct AVMuxerNapiAsyncContext : public MediaAsyncContext {
    explicit AVMuxerNapiAsyncContext(napi_env env) : MediaAsyncContext(env) {}
    ~AVMuxerNapiAsyncContext() = default;
    AVMuxerNapi *jsAVMuxer = nullptr;
    // napi_value instance_;
    std::string path_;
    std::string format_;
    // int32_t setOutputFlag_;
    MediaDescription trackDesc_;
    int32_t trackId_;
    // int32_t isAdd_;
    // int32_t isStart_;
    // napi_ref sample_;
    void *arrayBuffer_ = nullptr;
    size_t arrayBufferSize_;
    int32_t writeSampleFlag_;
    TrackSampleInfo trackSampleInfo_;
    // int32_t isStop_;
};

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
    CHECK_AND_RETURN_RET_LOG(status == napi_ok, nullptr, "Failed to set constructor");
    
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

napi_value AVMuxerNapi::CreateAVMuxer(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("CreateVideoPlayer In");

    std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "failed to napi_get_cb_info");
    }

    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);
    asyncContext->JsResult = std::make_unique<MediaJsResultInstance>(constructor_);
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "CreateAVMuxer", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    MEDIA_LOGD("success CreateAVMuxer");

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

void AVMuxerNapi::AsyncSetOutput(napi_env env, void *data)
{
    MEDIA_LOGD("AsyncSetOutput In");
    auto asyncContext = reinterpret_cast<AVMuxerNapiAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "AVMuxerNapiAsyncContext is nullptr!");

    if (asyncContext->jsAVMuxer == nullptr ||
        asyncContext->jsAVMuxer->avmuxerImpl_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsAVMuxer or avmuxerImpl_ is nullptr");
        return;
    }

    int32_t ret = asyncContext->jsAVMuxer->avmuxerImpl_->SetOutput(asyncContext->path_, asyncContext->format_);
    if (ret != MSERR_OK) {
        asyncContext->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "failed to SetVideoSurface");
    }
    MEDIA_LOGD("AsyncSetOutput Out");
}

napi_value AVMuxerNapi::SetOutput(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("SetOutput In");

    std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

    // get args
    napi_value jsThis = nullptr;
    napi_value args[3] = {nullptr};
    size_t argCount = 3;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "failed to napi_get_cb_info");
    }

    // get surface id from js
    napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr && napi_typeof(env, args[0], &valueType) == napi_ok && valueType == napi_string) {
        asyncContext->path_ = CommonNapi::GetStringArgument(env, args[0]);
    }
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_string) {
        asyncContext->format_ = CommonNapi::GetStringArgument(env, args[1]);
    }
    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[2]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);

    // get jsPlayer
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsAVMuxer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->jsAVMuxer != nullptr, result, "Failed to retrieve instance");
    
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "SetOutput", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, AVMuxerNapi::AsyncSetOutput,
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    MEDIA_LOGD("napi_queue_async_work Begin");
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    MEDIA_LOGD("napi_queue_async_work End");
    asyncContext.release();
    MEDIA_LOGD("SetOutput Out");
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

void AVMuxerNapi::AsyncAddTrack(napi_env env, void *data)
{
    MEDIA_LOGD("AsyncAddTrack In");
    auto asyncContext = reinterpret_cast<AVMuxerNapiAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "AsyncAddTrack is nullptr!");

    if (asyncContext->jsAVMuxer == nullptr ||
        asyncContext->jsAVMuxer->avmuxerImpl_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsAVMuxer or avmuxerImpl_ is nullptr");
        return;
    }

    int32_t ret = asyncContext->jsAVMuxer->avmuxerImpl_->AddTrack(asyncContext->trackDesc_, asyncContext->trackId_);
    MEDIA_LOGD("asyncContext->trackId_ is: %d", asyncContext->trackId_);
    asyncContext->JsResult = std::make_unique<MediaJsResultInt>(asyncContext->trackId_);
    if (ret != MSERR_OK) {
        asyncContext->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "failed to AsyncAddTrack");
    }
    MEDIA_LOGD("AsyncSetDisplaySurface Out");
}

napi_value AVMuxerNapi::AddTrack(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("AddTrack In");

    std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

    // get args
    napi_value jsThis = nullptr;
    napi_value args[2] = {nullptr};
    size_t argCount = 2;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "failed to napi_get_cb_info");
    }
    
    // get surface id from js
    // napi_valuetype valueType = napi_undefined;
    if (args[0] != nullptr) {
        // asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_TRACK_INDEX), GetNamedPropertyInt32(env, args[0], std::string(MD_KEY_TRACK_INDEX)));
        // asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_TRACK_TYPE), GetNamedPropertyInt32(env, args[0], std::string(MD_KEY_TRACK_TYPE)));
        // asyncContext->trackDesc_.PutStringValue(std::string(MD_KEY_CODEC_MIME), GetNamedPropertystring(env, args[0], std::string(MD_KEY_CODEC_MIME)));
        // asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_DURATION), GetNamedPropertyInt32(env, args[0], std::string(MD_KEY_DURATION)));
        // asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_BITRATE), GetNamedPropertyInt32(env, args[0], std::string(MD_KEY_BITRATE)));
        // asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_MAX_INPUT_SIZE), GetNamedPropertyInt32(env, args[0], std::string(MD_KEY_MAX_INPUT_SIZE)));
        // asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_WIDTH), GetNamedPropertyInt32(env, args[0], std::string(MD_KEY_WIDTH)));
        // asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_HEIGHT), GetNamedPropertyInt32(env, args[0], std::string(MD_KEY_HEIGHT)));
        // asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_PIXEL_FORMAT), GetNamedPropertyInt32(env, args[0], std::string(MD_KEY_PIXEL_FORMAT)));
        // asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_FRAME_RATE), GetNamedPropertyInt32(env, args[0], std::string(MD_KEY_FRAME_RATE)));
        // asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_CAPTURE_RATE), GetNamedPropertyInt32(env, args[0], std::string(MD_KEY_CAPTURE_RATE)));
        // asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_I_FRAME_INTERVAL), GetNamedPropertyInt32(env, args[0], std::string(MD_KEY_I_FRAME_INTERVAL)));
        // asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_REQUEST_I_FRAME), GetNamedPropertyInt32(env, args[0], std::string(MD_KEY_REQUEST_I_FRAME)));
        // asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_CHANNEL_COUNT), GetNamedPropertyInt32(env, args[0], std::string(MD_KEY_CHANNEL_COUNT)));
        // asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_SAMPLE_RATE), GetNamedPropertyInt32(env, args[0], std::string(MD_KEY_SAMPLE_RATE)));
        // asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_TRACK_COUNT), GetNamedPropertyInt32(env, args[0], std::string(MD_KEY_TRACK_COUNT)));
        // asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_CUSTOM_PREFIX), GetNamedPropertyInt32(env, args[0], std::string(MD_KEY_CUSTOM_PREFIX)));
        if (num == 0) {
            asyncContext->trackDesc_.PutStringValue(std::string(MD_KEY_CODEC_MIME), "video/mpeg4");
        } else {
            asyncContext->trackDesc_.PutStringValue(std::string(MD_KEY_CODEC_MIME), "audio/aac");
        }
        asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_WIDTH), 720);
        asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_HEIGHT), 480);
        asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_FRAME_RATE), 60);
        asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_CHANNEL_COUNT), 2);
        asyncContext->trackDesc_.PutIntValue(std::string(MD_KEY_SAMPLE_RATE), 44100);
    }
    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[1]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);
    
    // get jsPlayer
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsAVMuxer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->jsAVMuxer != nullptr, result, "Failed to retrieve instance");
    
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "AddTrack", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, AVMuxerNapi::AsyncAddTrack,
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    num++;

    return result;
}

void AVMuxerNapi::AsyncStart(napi_env env, void *data)
{
    MEDIA_LOGD("AsyncStart In");
    auto asyncContext = reinterpret_cast<AVMuxerNapiAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "AsyncStart is nullptr!");

    if (asyncContext->jsAVMuxer == nullptr ||
        asyncContext->jsAVMuxer->avmuxerImpl_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsAVMuxer or avmuxerImpl_ is nullptr");
        return;
    }

    int32_t ret = asyncContext->jsAVMuxer->avmuxerImpl_->Start();
    if (ret != MSERR_OK) {
        asyncContext->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "failed to AsyncStart");
    }
    MEDIA_LOGD("AsyncStart Out");
}

napi_value AVMuxerNapi::Start(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("Start In");

    std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "failed to napi_get_cb_info");
    }

    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);
    
    // get jsAVMuxer
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsAVMuxer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->jsAVMuxer != nullptr, result, "Failed to retrieve instance");
    
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Start", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, AVMuxerNapi::AsyncStart,
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();

    return result;
}

void AVMuxerNapi::AsyncWriteTrackSample(napi_env env, void *data)
{
    MEDIA_LOGD("AsyncWriteTrackSample In");
    auto asyncContext = reinterpret_cast<AVMuxerNapiAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "AsyncWriteTrackSample is nullptr!");

    if (asyncContext->jsAVMuxer == nullptr ||
        asyncContext->jsAVMuxer->avmuxerImpl_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsAVMuxer or avmuxerImpl_ is nullptr");
        return;
    }

    // napi_value sampleData = nullptr;
    // MEDIA_LOGD("begin arraybuffer");
    // napi_get_reference_value(env, asyncContext->sample_, &sampleData);
    // MEDIA_LOGD("begin arraybuffer1");
    // napi_get_arraybuffer_info(env, sampleData, &(asyncContext->arrayBuffer_), &(asyncContext->arrayBufferSize_));
    // std::shared_ptr<AVMemory> avMem = std::make_shared<AVMemory>(static_cast<uint8_t*>(asyncContext->arrayBuffer_), asyncContext->arrayBufferSize_);
    // avMem->SetRange(asyncContext->trackSampleInfo_.offset, asyncContext->trackSampleInfo_.size);
    // MEDIA_LOGD("size is: %{public}d", context->trackSampleInfo_.size);
    // MEDIA_LOGD("offset is: %{public}d", context->trackSampleInfo_.offset);
    // MEDIA_LOGD("flags is: %{public}d", context->trackSampleInfo_.flags);
    // MEDIA_LOGD("timeUs is: %{public}lld", context->trackSampleInfo_.timeUs);
    // MEDIA_LOGD("trackIdx is: %{public}d", context->trackSampleInfo_.trackIdx);
    // MEDIA_LOGD("data[0] is: %{public}u, data[1] is: %{public}u, data[2] is: %{public}u, data[3] is: %{public}u,", ((uint8_t*)(asyncContext->arrayBuffer_))[0], ((uint8_t*)(asyncContext->arrayBuffer_))[1], ((uint8_t*)(asyncContext->arrayBuffer_))[2], ((uint8_t*)(asyncContext->arrayBuffer_))[3]);
    // int32_t ret = asyncContext->jsAVMuxer->avmuxerImpl_->WriteTrackSample(avMem, asyncContext->trackSampleInfo_);
    // if (ret != MSERR_OK) {
    //     asyncContext->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "failed to AsyncWriteTrackSample");
    // }
    // napi_delete_reference(env, asyncContext->sample_);
    MEDIA_LOGD("AsyncWriteTrackSample Out");
}

napi_value AVMuxerNapi::WriteTrackSample(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("WriteTrackSample In");

    std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

    napi_value jsThis = nullptr;
    napi_value args[3] = {nullptr,};
    size_t argCount = 3;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && jsThis != nullptr,
        result, "Failed to retrieve details about the callback");
    
    napi_valuetype valueType = napi_undefined;
    bool isArrayBuffer;
    if (args[0] != nullptr && napi_is_arraybuffer(env, args[0], &isArrayBuffer) == napi_ok && isArrayBuffer == true) {
        napi_get_arraybuffer_info(env, args[0], &(asyncContext->arrayBuffer_), &(asyncContext->arrayBufferSize_));
        // napi_create_reference(env, args[0], 1, &asyncContext->sample_);
    }
    if (args[1] != nullptr && napi_typeof(env, args[1], &valueType) == napi_ok && valueType == napi_object) {
        napi_value trackSampleInfo;
        napi_get_named_property(env, args[1], PROPERTY_KEY_SAMPLEINFO.c_str(), &trackSampleInfo);
        asyncContext->trackSampleInfo_.size = GetNamedPropertyInt32(env, trackSampleInfo, PROPERTY_KEY_SIZE);
        asyncContext->trackSampleInfo_.offset = GetNamedPropertyInt32(env, trackSampleInfo, PROPERTY_KEY_OFFSET);
        asyncContext->trackSampleInfo_.flags = static_cast<FrameFlags>(GetNamedPropertyInt32(env, trackSampleInfo, PROPERTY_KEY_FLAG));
        asyncContext->trackSampleInfo_.timeUs = GetNamedPropertyInt64(env, trackSampleInfo, PROPERTY_KEY_TIMEUS);
        asyncContext->trackSampleInfo_.trackIdx = GetNamedPropertyInt32(env, args[1], PROPERTY_KEY_TRACK_ID);
        MEDIA_LOGD("size is: %{public}d", asyncContext->trackSampleInfo_.size);
        MEDIA_LOGD("offset is: %{public}d", asyncContext->trackSampleInfo_.offset);
        MEDIA_LOGD("flags is: %{public}d", asyncContext->trackSampleInfo_.flags);
        MEDIA_LOGD("timeUs is: %{public}lld", asyncContext->trackSampleInfo_.timeUs);
        MEDIA_LOGD("trackIdx is: %{public}d", asyncContext->trackSampleInfo_.trackIdx);
    }
    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[2]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);

    // get jsAVMuxer
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsAVMuxer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->jsAVMuxer != nullptr, result, "Failed to retrieve instance");

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
    asyncContext->writeSampleFlag_ = asyncContext->jsAVMuxer->avmuxerImpl_->WriteTrackSample(avMem, asyncContext->trackSampleInfo_);

    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, AVMuxerNapi::AsyncWriteTrackSample,
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();

    return result;
}

void AVMuxerNapi::AsyncStop(napi_env env, void *data)
{
    MEDIA_LOGD("AsyncStop In");
    auto asyncContext = reinterpret_cast<AVMuxerNapiAsyncContext *>(data);
    CHECK_AND_RETURN_LOG(asyncContext != nullptr, "AsyncStop is nullptr!");

    if (asyncContext->jsAVMuxer == nullptr ||
        asyncContext->jsAVMuxer->avmuxerImpl_ == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsAVMuxer or avmuxerImpl_ is nullptr");
        return;
    }

    int32_t ret = asyncContext->jsAVMuxer->avmuxerImpl_->Stop();
    if (ret != MSERR_OK) {
        asyncContext->SignError(MSERR_EXT_OPERATE_NOT_PERMIT, "failed to AsyncStop");
    }
    MEDIA_LOGD("AsyncStop Out");
}

napi_value AVMuxerNapi::Stop(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("Stop In");

    std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

    // get args
    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "failed to napi_get_cb_info");
    }

    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);

    // get jsPlayer
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsAVMuxer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->jsAVMuxer != nullptr, result, "Failed to retrieve instance");
    
    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Stop", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, AVMuxerNapi::AsyncStop,
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();

    return result;
}

// void AVMuxerNapi::AsyncRelease(napi_env env, void *data)
// {
// 	MEDIA_LOGD("AsyncRelease In");
// 	auto asyncContext = reinterpret_cast<AVMuxerNapiAsyncContext *>(data);
//     CHECK_AND_RETURN_LOG(asyncContext != nullptr, "AsyncRelease is nullptr!");

// 	if (asyncContext->jsAVMuxer == nullptr ||
//         asyncContext->jsAVMuxer->avmuxerImpl_ == nullptr) {
//         asyncContext->SignError(MSERR_EXT_NO_MEMORY, "jsAVMuxer or avmuxerImpl_ is nullptr");
//         return;
//     }

// 	asyncContext->jsAVMuxer->avmuxerImpl_->Release();
// 	MEDIA_LOGD("AsyncRelease Out");
// }

napi_value AVMuxerNapi::Release(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    napi_get_undefined(env, &result);
    MEDIA_LOGD("Release In");

    std::unique_ptr<AVMuxerNapiAsyncContext> asyncContext = std::make_unique<AVMuxerNapiAsyncContext>(env);
    CHECK_AND_RETURN_RET_LOG(asyncContext != nullptr, result, "Failed to create AVMuxerNapiAsyncContext instance");

    napi_value jsThis = nullptr;
    napi_value args[1] = {nullptr};
    size_t argCount = 1;
    napi_status status = napi_get_cb_info(env, info, &argCount, args, &jsThis, nullptr);
    if (status != napi_ok || jsThis == nullptr) {
        asyncContext->SignError(MSERR_EXT_NO_MEMORY, "failed to napi_get_cb_info");
    }

    asyncContext->callbackRef = CommonNapi::CreateReference(env, args[0]);
    asyncContext->deferred = CommonNapi::CreatePromise(env, asyncContext->callbackRef, result);

    // get jsAVMuxer
    status = napi_unwrap(env, jsThis, reinterpret_cast<void **>(&asyncContext->jsAVMuxer));
    CHECK_AND_RETURN_RET_LOG(status == napi_ok && asyncContext->jsAVMuxer != nullptr, result, "Failed to retrieve instance");
    
    asyncContext->jsAVMuxer->avmuxerImpl_->Release();

    napi_value resource = nullptr;
    napi_create_string_utf8(env, "Release", NAPI_AUTO_LENGTH, &resource);
    NAPI_CALL(env, napi_create_async_work(env, nullptr, resource, [](napi_env env, void* data) {},
        MediaAsyncContext::CompleteCallback, static_cast<void *>(asyncContext.get()), &asyncContext->work));
    NAPI_CALL(env, napi_queue_async_work(env, asyncContext->work));
    asyncContext.release();
    MEDIA_LOGD("Release Out");

    return result;
}

}  // namespace Media
}  // namespace OHOS
