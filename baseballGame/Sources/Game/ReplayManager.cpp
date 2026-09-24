#include "ReplayManager.h"
#include "ballDistance.h"

void ReplayManager::Initialize()
{
	replayFrames.clear();
	savedReplayList.clear();
	recordingStartTime = 0.0f;
	homeRunHitTime = 0.0f;
	saveTimeRemaining = 0.0f;
	recordElapsedTime = 0.0f;
	isRecording = false;
	isPendingSave = false;
}

void ReplayManager::Uninitialize()
{
	replayFrames.clear();
	savedReplayList.clear();
	recordingStartTime = 0.0f;
	homeRunHitTime = 0.0f;
	saveTimeRemaining = 0.0f;
	isRecording = false;
	isPendingSave = false;
	consoleLog = nullptr;
}

void ReplayManager::StartRecording(float startTime)
{
	replayFrames.clear();//リプレイデータをクリア
	recordingStartTime = startTime;//記録開始時間を設定
	homeRunHitTime = 0.0f;//ホームラン着弾時間をリセット
	saveTimeRemaining = 0.0f;//保存までの残り時間をリセット
	isRecording = true;//記録中フラグを立てる
	isPendingSave = false;//保存待機中フラグをリセット
	recordElapsedTime = 0.0f;//記録中の経過時間をリセット

	if(consoleLog)
	{
		consoleLog->push_back(u8"録画を開始しました");
	}
}

void ReplayManager::RecordFrame(const ReplayFrame& frame, float elapsedTime)
{
	if (isRecording)
	{
		recordElapsedTime += elapsedTime;//記録中の経過時間を更新
		ReplayFrame f = frame;
		f.time = recordElapsedTime;//フレームの時間を記録中の経過時間に設定
		replayFrames.push_back(f);//リプレイデータを追加
	}
}

void ReplayManager::OnHomeRunHit()
{
	if (isRecording)
	{
		homeRunHitTime = replayFrames.back().time;//ホームラン着弾時間を記録
		isPendingSave = true;//保存待機中フラグを立てる
		saveTimeRemaining = 2.0f;//2秒後に保存する

		if(consoleLog)
		{
			consoleLog->push_back(u8"ホームラン着弾！2秒後に録画を保存します");
		}
	}
}

void ReplayManager::OnHit()
{
	if (isRecording)
	{
		homeRunHitTime = replayFrames.back().time;//ヒット着弾時間を記録
		isPendingSave = true;//保存待機中フラグを立てる
		saveTimeRemaining = 2.0f;//2秒後に保存する
		if(consoleLog)
		{
			consoleLog->push_back(u8"ヒット着弾！2秒後に録画を保存します");
		}
	}
}

void ReplayManager::SaveRecording(float elapsedTime)
{
	if (isPendingSave)
	{
		saveTimeRemaining -= elapsedTime;//保存までの残り時間を減らす
		if (saveTimeRemaining <= 0.0f)
		{
			
			isPendingSave = false;//保存待機中フラグをリセット
			isRecording = false;//記録中フラグをリセット

			//最高飛距離を更新していたら録画データを保存する
			if(BallDistance::Instance().HasUpdatedMaxDistance())
			{
				//最高飛距離を更新していたら録画データを保存する
				savedReplayList = replayFrames;
				if(consoleLog)
				{
					consoleLog->push_back(u8"最高飛距離を更新したため、録画を保存しました");
				}
			}
			else
			{
				replayFrames.clear();//最高飛距離を更新していなければ保存しない
				if(consoleLog)
				{
					consoleLog->push_back(u8"最高飛距離を更新していないため、録画は保存されませんでした");
				}
			}

		}
	}
}

void ReplayManager::ClearRecording()
{
	replayFrames.clear();//リプレイデータをクリア
	recordingStartTime = 0.0f;//記録開始時間をリセット
	homeRunHitTime = 0.0f;//ホームラン着弾時間をリセット
	saveTimeRemaining = 0.0f;//保存までの残り時間をリセット
	isRecording = false;//記録中フラグをリセット
	isPendingSave = false;//保存待機中フラグをリセット

	if(consoleLog)
	{
		consoleLog->push_back(u8"録画をクリアしました");
	}
}

void ReplayManager::StartPlayback()
{
	if (!savedReplayList.empty())
	{
		isPlaying = true;
		playbackTime = 0.0f;
		playbackIndex = 0;
		currentPlaybackFrame = savedReplayList.front();//最初のフレームを設定
		if(consoleLog)
		{
			consoleLog->push_back(u8"リプレイ再生を開始しました");
		}
	}
	else
	{
		isPlaying = false;
		if(consoleLog)
		{
			consoleLog->push_back(u8"再生可能なリプレイがありません");
		}
		return;
	}
}

void ReplayManager::UpdatePlayback(float elapsedTime)
{
	if(!isPlaying || savedReplayList.empty()) return;

	playbackTime += elapsedTime;

	const float lastTime = savedReplayList.back().time;//最後のフレームの時間

	if (playbackTime >= lastTime)
	{
		if (isLoopPlayback)
		{
			playbackTime = 0.0f;
			playbackIndex = 0;
			currentPlaybackFrame = savedReplayList.front();
			if(consoleLog)
			{
				consoleLog->push_back(u8"リプレイ再生をループしました");
			}
		}
		else
		{
			playbackTime = lastTime;
			currentPlaybackFrame = savedReplayList.back();
			isPlaying = false;
			if(consoleLog)
			{
				consoleLog->push_back(u8"リプレイ再生が終了しました");
			}
			return;
		}
	}

	//現在時間が含まれる区間のフレームを探す
	while(playbackIndex + 1 < savedReplayList.size() && savedReplayList[playbackIndex + 1].time <= playbackTime)
	{
		playbackIndex++;
	}

	const ReplayFrame& a = savedReplayList[playbackIndex];

	if(playbackIndex + 1 < savedReplayList.size())
	{
		const ReplayFrame& b = savedReplayList[playbackIndex + 1];
		float t = (playbackTime - a.time) / (b.time - a.time);
		currentPlaybackFrame = LerpFrame(a, b, t);
	}
	else
	{
		currentPlaybackFrame = a;
	}
}

//2つのフレーム間を補間する関数
ReplayFrame ReplayManager::LerpFrame(const ReplayFrame& frame1, const ReplayFrame& frame2, float t)
{
	using namespace DirectX;

	ReplayFrame result;
	result.time = frame1.time + (frame2.time - frame1.time) * t;//時間の線形補間

	//位置の線形補間
	XMStoreFloat3(&result.pitcherPosition, XMVectorLerp(XMLoadFloat3(&frame1.pitcherPosition), XMLoadFloat3(&frame2.pitcherPosition), t));
	XMStoreFloat3(&result.pitcherRotation, XMVectorLerp(XMLoadFloat3(&frame1.pitcherRotation), XMLoadFloat3(&frame2.pitcherRotation), t));

	XMStoreFloat3(&result.batterPosition, XMVectorLerp(XMLoadFloat3(&frame1.batterPosition), XMLoadFloat3(&frame2.batterPosition), t));
	XMStoreFloat3(&result.batterRotation, XMVectorLerp(XMLoadFloat3(&frame1.batterRotation), XMLoadFloat3(&frame2.batterRotation), t));

	XMStoreFloat3(&result.ballPosition, XMVectorLerp(XMLoadFloat3(&frame1.ballPosition), XMLoadFloat3(&frame2.ballPosition), t));
	XMStoreFloat3(&result.ballVelocity, XMVectorLerp(XMLoadFloat3(&frame1.ballVelocity), XMLoadFloat3(&frame2.ballVelocity), t));
	XMStoreFloat3(&result.ballRotation, XMVectorLerp(XMLoadFloat3(&frame1.ballRotation), XMLoadFloat3(&frame2.ballRotation), t));

	XMStoreFloat3(&result.cameraEyePosition, XMVectorLerp(XMLoadFloat3(&frame1.cameraEyePosition), XMLoadFloat3(&frame2.cameraEyePosition), t));
	XMStoreFloat3(&result.cameraFocusPosition, XMVectorLerp(XMLoadFloat3(&frame1.cameraFocusPosition), XMLoadFloat3(&frame2.cameraFocusPosition), t));

	return result;
}