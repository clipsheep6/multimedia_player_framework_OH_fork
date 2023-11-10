/**
 * 单实例: 用于模块间传递是否首帧送显标志firstFlag
 *
 * @author c30053213
 * @since 2023年11月10日17:22:03
 */
class FirstRenderFlag{
public:
    static FirstRenderFlag *GetInstance();
    static void deleteInstance();
    void setFirstFlag(bool flag){
        this->firstFlag = flag;
    }
    bool getFirstFlag(){
        return this->firstFlag;
    }
private:
    FirstRenderFlag();
    ~FirstRenderFlag();

    FirstRenderFlag(const FirstRenderFlag &signal);
    const FirstRenderFlag &operator=(const FirstRenderFlag &signal);
private:
    static FirstRenderFlag *m_SingleInstance;
    static std::mutex m_Mutex;
private:
    bool firstFlag;
};