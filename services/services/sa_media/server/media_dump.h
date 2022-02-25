/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef MEDIA_STANDARD_MEDIA_DUMP_H
#define MEDIA_STANDARD_MEDIA_DUMP_H


#include <fstream>
#include <iostream>
#include <string>
#include "media_server_manager.h"
#include "media_client.h"
namespace OHOS{
namespace Media{
class MediaDump{
public:
    MediaDump();
    ~MediaDump();
    void GetClients(std::shared_ptr<MediaClient> &mediaClient);
    void GetServerManagers(std::shared_ptr<MediaServerManager> &mediaServerManager);
    void DumpClientInfo();
    void DumpServerInfo();
    void DumpLocalPlayerInfo( std::shared_ptr<IPlayerService> &playerServer) ;
    void DumLocalRecorderInfo(std::shared_ptr<IRecorderService> &recorderServer) ;
    void DumpLocalAVCodecInfo(std::shared_ptr<IAVCodecService> &avcodecServer) ;
    void DumpLocalAVMetadataHelperInfo(std::shared_ptr<IAVMetadataHelperService> &avMetadataHelper) ;

private:
    std::shared_ptr<MediaClient> mediaClient_;
    std::shared_ptr<MediaServerManager> mediaServerManager_;
    void DumpPlayerServerInfo(std::map<sptr<IRemoteObject>, pid_t> &playerStubMap);
    void DumpRecorderServerInfo(std::map<sptr<IRemoteObject>, pid_t> &recorderStubMap);
    void DumpAvcodecServerInfo(std::map<sptr<IRemoteObject>, pid_t> &avCodecStubMap);
    void DumpAvMetaDataHelperServerInfo(std::map<sptr<IRemoteObject>, pid_t> &avMetadataHelperStubMap);
    void DumpPlayerInfo( std::shared_ptr<IPlayerService> &playerServer);
    void DumpRecorderInfo(std::shared_ptr<IRecorderService> &recorderServer);
    void DumpAVCodecInfo(std::shared_ptr<IAVCodecService> &avcodecServer);
    void DumpAVMetadataHelperInfo(std::shared_ptr<IAVMetadataHelperService> &avMetadataHelper);
    void DumpFormatInfo(std::vector<Format> &formatData);
    long MaxOfList(std::list<long> &list);
    long AverageOfList(std::list<long> &list);
    std::string SetDumpFilename();
    ofstream dumpFile;
};
}
}
# endif //MEDIA_STANDARD_MEDIA_DUMP_H
