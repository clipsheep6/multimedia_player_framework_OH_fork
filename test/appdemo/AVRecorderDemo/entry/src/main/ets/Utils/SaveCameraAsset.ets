import fileio from '@ohos.fileio';
import mediaLibrary from '@ohos.multimedia.mediaLibrary';
import DateTimeUtil from './DateTimeUtils';

export default class SaveCameraAsset {
  private tag
  constructor(tag) {
    this.tag = tag;
  }

  private lastSaveTime = '';
  private saveIndex = 0;
  public VideoPrepareFile: any;

  public async createVideoFd(): Promise<number> {
    console.info(`${this.tag} get Recorder File Fd`);
    const mDateTimeUtil = new DateTimeUtil();
    const displayName = this.checkName(`REC_${mDateTimeUtil.getDate()}_${mDateTimeUtil.getTime()}`) + '.mp4';
    console.info(`${this.tag} get Recorder display name is: ` + displayName);
    const media = mediaLibrary.getMediaLibrary(globalThis.abilityContext)
    console.info(`${this.tag} get Recorder File Fd publicPath: ${media}`)
    const fileKeyObj = mediaLibrary.FileKey;
    const mediaType = mediaLibrary.MediaType.VIDEO;
    let publicPath: string = await media.getPublicDirectory(mediaLibrary.DirectoryType.DIR_VIDEO);
    console.info(`${this.tag} get Recorder File publicPath: ${JSON.stringify(publicPath)}`);

    try {
      const dataUri = await media.createAsset(mediaType, displayName, publicPath);
      console.info(`${this.tag} get Recorder dataUri is: ` + dataUri);
      if (dataUri != undefined) {
        console.info(`${this.tag} dataUri is undefined`);
        const args = dataUri.id.toString();
        const fetchOp = {
          selections : fileKeyObj.ID + "=?",
          selectionArgs : [args],
        }
        console.info(`${this.tag} fetchOp= ${JSON.stringify(fetchOp)}`)
        const fetchFileResult = await media.getFileAssets(fetchOp);
        console.info(`${this.tag} getFileAssets finished`);
        this.VideoPrepareFile = await fetchFileResult.getFirstObject();
        const fdNumber = await this.VideoPrepareFile.open('rw');
        console.info(`${this.tag} get Recorder File fd is: ` + fdNumber);
        return fdNumber
      }
    } catch(err) {
      console.error(`${this.tag} create Recorder File Fd ERROR: ` + err);
    }
  }

  private checkName(name:string): string {
    if (this.lastSaveTime == name) {
      this.saveIndex += 1;
      return `${name}_${this.saveIndex}`
    }
    this.lastSaveTime = name;
    this.saveIndex = 0;
    console.info(`${this.tag} get Recorder File name is: ` + name);
    return name;
  }
}