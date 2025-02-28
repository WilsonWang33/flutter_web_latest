package com.smwl.x7market;

import android.app.Activity;
import android.app.Application;
import android.content.Context;

import com.netease.nimlib.sdk.NIMClient;
import com.netease.nimlib.sdk.SDKOptions;
import com.netease.nimlib.sdk.sync.SyncConfig;

import java.io.File;

public class MyApplication extends Application {

    @Override
    public void onCreate() {
        super.onCreate();
        // SDK 初始化（启动后台服务，若已经存在用户登录信息，SDK 将进行自动登录）。不能对初始化语句添加进程判断逻辑。
        NIMClient.init(this, null, getSDKOptions(this, MainActivity.class));
    }



    public static SDKOptions getSDKOptions(Context context, Class<? extends Activity> notificationEntranceClass) {
        SDKOptions options = new SDKOptions();

        // 配置数据库加密秘钥
        options.databaseEncryptKey = "NETEASE";

        // 配置是否需要预下载附件缩略图
        options.preloadAttach = true;


        // 在线多端同步未读数
        options.sessionReadAck = true;

        // 动图的缩略图直接下载原图
        options.animatedImageThumbnailEnabled = true;

        // 采用异步加载SDK
        options.asyncInitSDK = true;

        // 是否是弱IM场景
        options.reducedIM = false;

        // 是否检查manifest 配置，调试阶段打开，调试通过之后请关掉
        options.checkManifestConfig = false;

        // 是否启用群消息已读功能，默认关闭
        options.enableTeamMsgAck = true;


        // 打开消息撤回未读数-1的开关
        options.shouldConsiderRevokedMessageUnreadCount = true;

        //禁止后台进程唤醒UI进程
        options.disableAwake = true;

        options.notifyStickTopSession = true;

        options.syncConfig = new SyncConfig.Builder().setEnableSyncTeamMember(false).setEnableSyncSuperTeamMember(false)
                .setEnableSyncSuperTeamMemberUserInfo(false).setEnableSyncTeamMemberUserInfo(false).build();


        //谢注释，这个是需要在小米，华为，魅族官网注册相关账户后的系统推送2018.12.8
        //options.mixPushConfig = buildMixPushConfig();

//        options.mNosTokenSceneConfig = createNosTokenScene();

//        options.loginCustomTag = "登录自定义字段";
        return options;
    }
}
