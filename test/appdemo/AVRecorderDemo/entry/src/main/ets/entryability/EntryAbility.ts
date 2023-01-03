import hilog from '@ohos.hilog';
import Ability from '@ohos.application.Ability';
import Window from '@ohos.window';

/**
 * Lift cycle management of Ability.
 */
export default class EntryAbility extends Ability {
    async onCreate(want, launchParam) {
        hilog.isLoggable(0x0000, 'testTag', hilog.LogLevel.INFO);
        hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onCreate');
        console.log("[AVRecorder] MainAbility onCreate");
        let array: Array<string> = [
            'ohos.permission.MEDIA_LOCATION',
            'ohos.permission.READ_MEDIA',
            'ohos.permission.WRITE_MEDIA',
            'ohos.permission.CAMERA',
            'ohos.permission.MICROPHONE',
        ]
        await this.context.requestPermissionsFromUser(array).then(function(data) {
            console.log("data permissions:" + data.permissions);
            console.log("data result:" + data.authResults);
        }, (err) => {
            console.error("data result:" + err.code);
        })
        globalThis.abilityWant = want;
        globalThis.abilityContext = this.context;

    }

    onDestroy() {
        hilog.isLoggable(0x0000, 'testTag', hilog.LogLevel.INFO);
        hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onDestroy');
    }

    onWindowStageCreate(windowStage: Window.WindowStage) {
        // Main window is created, set main page for this ability
        hilog.isLoggable(0x0000, 'testTag', hilog.LogLevel.INFO);
        hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onWindowStageCreate');

//        // 申请权限
//        let array: Array<string> = [
//            'ohos.permission.MEDIA_LOCATION',
//            'ohos.permission.READ_MEDIA',
//            'ohos.permission.WRITE_MEDIA',
//            'ohos.permission.CAMERA',
//            'ohos.permission.MICROPHONE',
//        ]
//        this.context.requestPermissionsFromUser(array).then(function(data) {
//            console.log("data permissions:" + data.permissions);
//            console.log("data result:" + data.authResults);
//        }, (err) => {
//            console.error("data result:" + err.code);
//        })

        windowStage.loadContent("pages/ListPage", (err, data) => {
            if (err.code) {
                hilog.isLoggable(0x0000, 'testTag', hilog.LogLevel.ERROR);
                hilog.error(0x0000, 'testTag', 'Failed to load the content. Cause: %{public}s', JSON.stringify(err) ?? '');
                return;
            }
            hilog.isLoggable(0x0000, 'testTag', hilog.LogLevel.INFO);
            hilog.info(0x0000, 'testTag', 'Succeeded in loading the content. Data: %{public}s', JSON.stringify(data) ?? '');
        });
    }

    onWindowStageDestroy() {
        // Main window is destroyed, release UI related resources
        hilog.isLoggable(0x0000, 'testTag', hilog.LogLevel.INFO);
        hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onWindowStageDestroy');
    }

    onForeground() {
        // Ability has brought to foreground
        hilog.isLoggable(0x0000, 'testTag', hilog.LogLevel.INFO);
        hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onForeground');
    }

    onBackground() {
        // Ability has back to background
        hilog.isLoggable(0x0000, 'testTag', hilog.LogLevel.INFO);
        hilog.info(0x0000, 'testTag', '%{public}s', 'Ability onBackground');
    }
}