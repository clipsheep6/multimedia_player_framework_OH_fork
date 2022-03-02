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
#include "media_dump.h"
#include <ctime>
#include "format.h"
#include "player_server.h"
#include "recorder_server.h"
#include "avcodec_server.h"
#include "avmetadatahelper_server.h"


namespace OHOS {
namespace Media {
MediaDump::MediaDump()
{
    MEDIA_LOGD("Media Information Dump");
}

MediaDump::~MediaDump()
{
}

void MediaDump::GetClients(std::shared_ptr<MediaClient> &mediaClient)
{
    mediaClient_ = mediaClient;
}

void MediaDump::GetServerManagers(std::shared_ptr<MediaServerManager> &mediaServerManager)
{
    mediaServerManager_ = mediaServerManager;
}

void MediaDump::DumpClientInfo()
{
    dumpFle.open(SetDumpFilename(),ios::out || ios::app);
    mediaClient_.GetClientLists();
    dumpFile << "Current client information:" <<std::endl;
    dumpFile << "Player client quantity : " << mediaClient_.playerClientList.size()<<std::endl;
    dumpFile << "Recorder client quantity : " << mediaClient_.playerClientList.size()<<std::endl;
    dumpFile << "Avcodec client quantity : " << mediaClient_.playerClientList.size()<<std::endl;
    dumpFile <<"Avmetadatahelper client quantity : " <<mediaClient_.playerClientList.size()<<std::endl;
    dumpFile.close();
}

void MediaDump::DumpServerInfo()
{
    dumpFle.open(SetDumpFilename(),ios::out || ios::app);
    mediaServerManager_.GetServiceStubMaps();
    DumpPlayerServerInfo(MediaServerManager_.playerStubMap);
    DumpRecorderServerInfo(MediaServerManager_.recorderStubMap);
    DumpAvcodecServerInfo(MediaServerManager_.avCodecStubMap);
    DumpAvMetaDataHelperServerInfo(MediaServerManager_.avMetadataHelperStubMap);
    dumpFile.close();
}

void MediaDump::DumpLocalPlayerInfo( std::shared_ptr<IPlayerService> &playerServer)
{
    dumpFle.open(SetDumpFilename(),ios::out || ios::app);
    dumpFile << "Dump local player information : " << std::endl;
    DumpPlayerInfo(playerServer);
    dumpFile.close();

}

void MediaDump::DumLocalRecorderInfo(std::shared_ptr<IRecorderService> &recorderServer)
{
    dumpFle.open(SetDumpFilename(),ios::out || ios::app);
    dumpFile << "Dump local recorder information : " << std::endl;
    DumpRecorderInfo(recorderServer);
    dumpFile.close();
}

void MediaDump::DumpLocalAVCodecInfo(std::shared_ptr<IAVCodecService> &avcodecServer)
{
    dumpFle.open(SetDumpFilename(),ios::out || ios::app);
    dumpFile << "Dump local avCodec information : " << std::endl;
    DumpAVCodecInfo(avcodecServer);
    dumpFile.close();
}

void MediaDump::DumpLocalAVMetadataHelperInfo(std::shared_ptr<IAVMetadataHelperService> &avMetadataHelper)
{
    dumpFle.open(SetDumpFilename(),ios::out || ios::app);
    dumpFile << "Dump local aVMetadataHelper information : " << std::endl;
    DumpAVMetadataHelperInfo(avMetadataHelper);
    dumpFile.close();
}

void MediaDump::DumpPlayerInfo( std::shared_ptr<IPlayerService> &playerServer)
{
    pid_t pid = playerServer.pid;
    pid_t uid = playerServer.uid;
    PlayerStates status = playerServer.status;
    std::string  playerSourceUrl = playerServer.playerSourceUrl;
    int32_t currentTime;
    int32_t extra;
    Format &infoBody;
    std::vector<Format> videoTrack;
    std::vector<Format> audioTrack;
    playerServer.GetCurrentTime(currentTime);
    playerServer.GetAudioTrackInfo(audioTrack);
    playerServer.GetVideoTrackInfo(videoTrack);
    playerServer.OnInfo(INFO_TYPE_BUFFERING_UPDATE, extra, infoBody);
    playerServer.GetBufferPercentValue();
    int32_t bufferPercentValue = playerServer.bufferPercentValue;
    std::list<long>  startPlayTimesList = playerServer.startPlayTimesList;
    std::list<long>  seekTimesList = playerServer.seekTimesList;
    long nearestTimeOfStartPlay = *startPlayTimesList.end();
    long averageTimeOfStartPlay = AverageOfList(startPlayTimesList);
    long maxTimeOfStartPlay = MaxOfList(startPlayTimesList);
    long nearestTimeOfSeek = *seekTimesList.end();
    long averageTimeOSeek = AverageOfList(seekTimesList);
    long maxTimeOfSeek = MaxOfList(seekTimesList);
    dumpFile << "UID : " << uid << "PID : " << pid << "player status : " << status << std::endl;
    dumpFile << "player url ： " <<  playerSourceUrl << "player location : " << currentTime
             << std::endl;
    dumpFile << "audio track Information : " << std::endl;
    DumpFormatInfo(audioTrack);
    dumpFile << "video track Information : " << std::endl;
    DumpFormatInfo(videoTrack);
    dumpFile << "buffer Information : " <<  bufferPercentValue << "%" << std::endl;
    dumpFile << "the performance of start play: " << std::endl;
    dumpFile << "nearest time : " << nearestTimeOfStartPlay << "max time : " << maxTimeOfStartPlay <<
             "average time : " << averageTimeOfStartPlay << std::endl;
    dumpFile << "nearest time : " << nearestTimeOfSeek << "max time : " << maxTimeOfSeek <<
             "average time : " << averageTimeOSeek << std::endl;
}

void MediaDump::DumpRecorderInfo(std::shared_ptr<IRecorderService> &recorderServer)
{
    pid_t pid = recorderServer.pid;
    pid_t uid = recorderServer.uid;
    RecStatus status = recorderServer.status;
    std::string outPutPath = recorderServer.outPutPath;
    RecorderParameter recParameter = recorderServer.recParameter;
    dumpFile << "UID : " << uid << "PID : " << pid << "recorder status : " << status << std::endl;
    dumpFile << "Recorder outPut Path : " << outPutPath << std::endl;
    dumpFile << "Recoder parameter : " << std::endl;
    dumpFile <<  "video Width : " << recParameter.videoWidth << "video height" << recParameter.videoHeight
             << "videoFrameRate ： " << recParameter.videoFrameRate << "videoEncodingBitRate : " <<
             "captureRate ：" << recParameter.captureRate << "audioSampleRate : " << recParameter.audioSampleRate <<
             "audioChannelsQuantity : " << recParameter.audioChannelsQuantity << "audioEncodingBitRate : "<<
             recParameter.audioEncodingBitRate << std::endl;
}

void MediaDump::DumpAVCodecInfo(std::shared_ptr<IAVCodecService> &avcodecServer)
{
    pid_t pid = avcodecServer.pid;
    pid_t uid = avcodecServer.uid;
    AVCodecStatus status = avcodecServer.status;
    AVCodecType avcodecType = avcodecServer.avcodecType;
    bool mimeType = avcodecServiceStub.avcodecServer.mimeType;
    dumpFile << "UID : " << uid << "PID : " << pid << "avcodec status : " << status << std::endl;
    dumpFile << "Avcodec Type : " << avcodecType << "Is mime Type ? : " << mimeType <<  std::endl;
}

void MediaDump::DumpAVMetadataHelperInfo(std::shared_ptr<IAVMetadataHelperService> &avMetadataHelper)
{
    pid_t pid = avMetadataHelperServer.pid;
    pid_t uid = avMetadateHelperServer.uid;
    std::string  avMetaDataHelperSourceUrl = avMetadateHelperServer.avMetaDataHelperSourceUrl;
    std::list<long>  resolveMetaDataTimeList = avMetadateHelperServer.resolveMetaDataTimeList;
    std::list<long>  fetchThumbnailTimeList = avMetadateHelperServer.fetchThumbnailTimeList;
    long nearestTimeOfResolveMetaData = *resolveMetaDataTimeList.end();
    long averageTimeOfResolveMetaData = AverageOfList(resolveMetaDataTimeList);
    long maxTimeOfResolveMetaData = MaxOfList(resolveMetaDataTimeList);
    long nearestTimeOfFetchThumbnail = *fetchThumbnailTimeList.end();
    long averageTimeOfFetchThumbnail = AverageOfList(fetchThumbnailTimeList);
    long maxTimeOfFetchThumbnail = MaxOfList(fetchThumbnailTimeList);
    dumpFile << "UID : " << uid << "PID : " << pid << "avMetaDataHelper SourceUrl : "
        << avMetaDataHelperSourceUrl << std::endl;
    dumpFile << "the performance of resolveMetaData : " << std::endl;
    dumpFile << "nearest time : " << nearestTimeOfResolveMetaData << "max time : " << maxTimeOfResolveMetaData <<
             "average time : " << averageTimeOfResolveMetaData << std::endl;
    dumpFile << "nearest time : " << nearestTimeOfFetchThumbnail << "max time : " << maxTimeOfFetchThumbnail <<
             "average time : " << averageTimeOfFetchThumbnail << std::endl;
}

void MediaDump::DumpPlayerServerInfo(std::map<sptr<IRemoteObject>, pid_t> &playerStubMap)
{
    int i = 1;
    for (auto iter = playerStubMap.begin(); iter !=playerStubMap.end(); iter++) {
        sptr<PlayerServiceStub> playerServiceStub = iter->first;
        playerServiceStub.GetPlayerServer();
        std::shared_ptr<IPlayerService> playerServer = playerServiceStub.playerServer;
        dumpFile << "Player Client " << i << "information : " << std::endl;
        if (playerServer != nullptr) {
            DumpPlayerInfo(playerServer);
        }
        i++;
    }
}

void MediaDump::DumpRecorderServerInfo(std::map<sptr<IRemoteObject>, pid_t> &recorderStubMap)
{
    int i = 1;
    for (auto iter = recorderStubMap.begin(); iter !=recorderStubMap.end(); iter++) {
        sptr<RecorderServiceStub> recorderServiceStub = iter->first;
        recorderServiceStub.GetRecoderServer();
        std::shared_ptr<IRecorderService> recorderServer = recorderServiceStub.recorderServer;
        dumpFile << "Recorde Client " << i << "information : " << std::endl;
        if (recorderServer != nullptr) {
            DumpRecorderInfo(recorderServer);
        }
        i++;
    }
}

void MediaDump::DumpAvcodecServerInfo(std::map<sptr<IRemoteObject>, pid_t> &avCodecStubMap)
{
    int i = 1;
    for (auto iter =avCodecStubMap.begin(); iter !=avCodecStubMap.end(); iter++) {
        sptr<AVCodecServiceStub> avcodecServiceStub = iter->first;
        avcodecServiceStub.GetAvcodecServer();
        std::shared_ptr<IAVCodecService> avcodecServer = avcodecServiceStub.avcodecServer;
        dumpFile << "Avcodec Client " << i << "information : " << std::endl;
        if (recorderServer != nullptr) {
            DumpAVCodecInfo(avcodecServer);
        }
        i++;
    }
}

void MediaDump::DumpAvMetaDataHelperServerInfo(std::map<sptr<IRemoteObject>, pid_t> &avMetadataHelperStubMap)
{
    int i = 1;
    for (auto iter = avMetadataHelperStubMap.begin(); iter !=avMetadataHelperStubMap.end(); iter++) {
        sptr<AVMetadataHelperServiceStub avMetadataHelperServiceStub = iter->first;
        avMetadataHelperServiceStub.GetAvMetadataHelperServer();
        std::shared_ptr<IAVMetadataHelperService> avMetadataHelperServer =
             avMetadataHelperServiceStub.avMetadateHelperServer;
        dumpFile << "AvMetaDataHelper Client " << i << "information : " << std::endl;
        if (recorderServer != nullptr) {
            DumpAVMetadataHelperInfo(avMetadataHelperServer);
        }
        i++;
    }
}

void MediaDump::DumpFormatInfo(std::vector<Format> &formatData)
{
    for (auto vectorIter = formatData.begin(); vectorIter != formatData.end(); vectorIter++) {
        Format data = *vectorIter;
        for(auto mapIter = data.FormatDataMap.begin(); mapIter != data.FormatDataMap.end(); mapIter++) {
            std::string string = data.FormatDataMap->first;
            FormatData  formatData = data.FormatDataMap->second;
            dumpFile << "Format type : " << string << std::endl;
            dumpFile << "Format data : " << std::endl;
            dumpFile <<  "    " << "Format data type : "  << formatData.type << " Format data stringVal : "
                << formatData.stringVal << "Format data address : " << formatData.addr << " Format data size : "
                << formatData.size << std:: endl;
        }
    }
}

long MediaDump::MaxOfList(std::list<long> &list)
{
    long max = 0;
    for (auto iter = list.begin(); iter != list.end(); iter++) {
        if(*iter > max || *iter == max) {
            max = *iter;
        }
    }
    return max;
}

long MediaDump::AverageOfList(std::list<long> &list)
{
    long sum = 0;
    int32_t num = list.size();
    for (auto iter = list.begin(); iter != list.end(); iter++) {
       sum += *iter;
    }
    if (num != 0) {
        long average = sum/num;
    }
    return average;
}

std::string MediaDump::SetDumpFilename();
{
    time_t now = time(0);
    tm *ltm = localtime(&now);
    std::string fileName;
    fileName = "Dump" + std::to_string(1900 + ltm->tm_year) + std::to_string(1 + ltm->tm_month)
               + std::to_string(ltm->tm_mday) + std::to_string(ltm->tm_min) + std::to_string(ltm->tm_sec)
               + ".log"
    return fileName;
}
} // Media
} // OHOS