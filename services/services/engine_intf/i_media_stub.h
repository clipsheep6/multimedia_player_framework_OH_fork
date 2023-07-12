#ifndef I_MEDIA_STUB_H
#define I_MEDIA_STUB_H
#include "iremote_stub.h"
#include "iremote_broker.h"
namespace OHOS {
namespace Media {
class IMediaStub : public IRemoteBroker {
public:
    virtual int32_t DumpInfo(int32_t fd)
    {
        (void)fd;
        return 0;
    };
};
}
}
#endif