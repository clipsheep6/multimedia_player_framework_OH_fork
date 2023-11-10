/**
 * 单实例: 用于模块间传递是否首帧送显标志firstFlag
 *
 * @author c30053213
 * @since 2023年11月10日17:30:33
 */
#include "first_render_flag.h"

FirstRenderFlag *FirstRenderFlag::m_SingleInstance = nullptr;
std::mutex FirstRenderFlag::m_Mutex;

FirstRenderFlag * FirstRenderFlag::GetInstance()
{
    if (m_SingleInstance == nullptr)
    {
        std::unique_lock<std::mutex> lock(m_Mutex);
        if (m_SingleInstance == nullptr)
        {
            volatile auto temp = new (std::nothrow) FirstRenderFlag();
            m_SingleInstance = temp;
        }
    }

    return m_SingleInstance;
}

void FirstRenderFlag::deleteInstance()
{
    std::unique_lock<std::mutex> lock(m_Mutex);
    if (m_SingleInstance)
    {
        delete m_SingleInstance;
        m_SingleInstance = nullptr;
    }
}