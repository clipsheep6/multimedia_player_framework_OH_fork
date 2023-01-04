/*
 * Copyright (C) 2022-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the 'License');
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an 'AS IS' BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

export default class DateTimeUtil {
  /* 时分秒 */
  getTime() {
    const DATETIME = new Date();

    return this.concatTime(DATETIME.getHours(), DATETIME.getMinutes(), DATETIME.getSeconds())
  }

  /* 年月日 */
  getDate() {
    const DATETIME = new Date();
    return this.concatDate(DATETIME.getFullYear(), DATETIME.getMonth()+1, DATETIME.getDate())
  }

  /* 日期不足两位补0 */
  fill(value: number) {
    return (value > 9 ? '':'0') + value;
  }

  /* 年月日格式修饰 */
  concatDate(year: number, month: number, date: number) {
    return `${year}${month}${date}`;
  }

  /* 时分秒格式修饰 */
  concatTime(hour: number, minute: number, second: number) {
    return `${this.fill(hour)}${this.fill(minute)}${this.fill(second)}`;
  }
}

/* 毫秒 转换为 分秒 */
export function getShownTimer(ms) {
  let seconds: any = Math.round(ms / 1000); // 毫秒转化为秒
  let sec: any = seconds %60;
  let min: any = (seconds - sec) / 60;
  if (sec < 10) {
    sec = '0' + sec;
  }
  if (min < 10) {
    min = '0' + min;
  }
  return min + ':' + sec;
}

/* 毫秒 转换为 时分秒毫秒 */
export function dateTime(t) {
  const millisecond: any = t % 1000
  let second: any = 0;
  let totalMinute: any = 0;
  let minute: any = 0;
  let hour: any = 0;
  let result: any = "";
  const totalSecond = Math.floor(t/1000)
  if (totalSecond > 59) {
    second = totalSecond % 60;
    totalMinute = Math.floor(totalSecond/60);
  } else {
    second = totalSecond;
  }
  if (totalMinute > 59) {
    minute = totalMinute % 60;
    hour = Math.floor(totalMinute/60);
  } else {
    minute = totalMinute;
  }
  if(hour < 10) {
    result += '0' + hour;
  } else {
    result += '' + hour;
  }
  if(minute < 10) {
    result += ':0' + minute;
  } else {
    result += ':' + minute;
  }
  if(second < 10) {
    result += ':0' + second;
  } else {
    result += ':' + second;
  }
  if(millisecond < 10) {
    result += ',00' + millisecond;
  } else if (millisecond < 100) {
    result += ',0' + millisecond;
  } else {
    result += ',' + millisecond;
  }
  return result;
}