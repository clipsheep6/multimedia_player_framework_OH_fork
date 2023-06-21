/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "test_player.h"
#include <iostream>
#include "string_ex.h"
#include "media_errors.h"
#include "directory_ex.h"
#include "ui/rs_surface_node.h"
#include "window_option.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;

void TestPlayerCallback::OnError(int32_t errorCode, const std::string &errorMsg)
{
    cout << "Error received, errorCode: " << errorCode << "errorMsg: " << errorMsg << endl;
}

void TestPlayerCallback::OnInfo(PlayerOnInfoType type, int32_t extra, const Format &infoBody)
{
    (void)infoBody;
    switch (type) {
        case INFO_TYPE_SEEKDONE:
            cout << "PlayerCallback: OnSeekDone currentPositon is " << extra << endl;
            break;
        case INFO_TYPE_SPEEDDONE:
            cout << "PlayerCallback: SpeedDone " << endl;
            break;
        case INFO_TYPE_EOS:
            cout << "PlayerCallback: OnEndOfStream isLooping is " << extra << endl;
            break;
        case INFO_TYPE_BUFFERING_UPDATE:
            break;
        case INFO_TYPE_STATE_CHANGE:
            break;
        case INFO_TYPE_POSITION_UPDATE:
            cout << "OnPositionUpdated position is " << extra << endl;
            break;
        case INFO_TYPE_MESSAGE:
            cout << "PlayerCallback: OnMessage is " << extra << endl;
            break;
        case INFO_TYPE_RESOLUTION_CHANGE:
            break;
        case INFO_TYPE_VOLUME_CHANGE:
            cout << "PlayerCallback: volume changed" << endl;
            break;
        default:
            break;
    }
}
TestPlayer::TestPlayer()
{
}

TestPlayer::~TestPlayer()
{
    if (previewWindow_ != nullptr) {
        previewWindow_->Destroy();
        previewWindow_ = nullptr;
    }
}
sptr<Surface> TestPlayer::GetVideoSurface()
{
    sptr<Rosen::WindowOption> option = new Rosen::WindowOption();
    option->SetWindowRect({ 0, 0, 640, 480 });
    option->SetWindowType(Rosen::WindowType::WINDOW_TYPE_APP_LAUNCHING);
    option->SetWindowMode(Rosen::WindowMode::WINDOW_MODE_FLOATING);
    previewWindow_ = Rosen::Window::Create("xcomponent_window_fuzztest", option);
    if (previewWindow_ == nullptr || previewWindow_->GetSurfaceNode() == nullptr) {
        cout << "previewWindow_ is nullptr" << endl;
        return nullptr;
    }
    previewWindow_->Show();
    return previewWindow_->GetSurfaceNode()->GetSurface();
}

int32_t TestPlayer::SetFdSource(const string &path)
{
    int32_t fdValue = open(path.c_str(), O_RDONLY);
    if (fdValue < 0) {
        cout << "Open file failed" << endl;
        (void)close(fdValue);
        return -1;
    }
    int32_t offsetValue = 0;

    struct stat64 buffer;
    if (fstat64(fdValue, &buffer) != 0) {
        cout << "Get file state failed" << endl;
        (void)close(fdValue);
        return -1;
    }
    int64_t lengthValue = static_cast<int64_t>(buffer.st_size);
    int32_t retValue = player_->SetSource(fdValue, offsetValue, lengthValue);
    if (retValue != 0) {
        (void)close(fdValue);
        return -1;
    }
    (void)close(fdValue);
    return 0;
}
