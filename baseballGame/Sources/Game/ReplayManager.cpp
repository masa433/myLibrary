#include "ReplayManager.h"
#include "ballDistance.h"
#include "HomeRunCount.h"
#include <imgui.h>

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
	hasLooped = false;
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
		saveTimeRemaining = 1.0f;//1秒後に保存する

		if(consoleLog)
		{
			consoleLog->push_back(u8"ホームラン着弾！1秒後に録画を保存します");
		}
	}
}

void ReplayManager::OnHit()
{
	if (isRecording)
	{
		homeRunHitTime = replayFrames.back().time;//ヒット着弾時間を記録
		isPendingSave = true;//保存待機中フラグを立てる
		saveTimeRemaining = 1.0f;//1秒後に保存する
		if(consoleLog)
		{
			consoleLog->push_back(u8"ヒット着弾！1秒後に録画を保存します");
		}
	}
}

void ReplayManager::SaveRecording(float elapsedTime)
{
	if (isPendingSave)
	{
		saveTimeRemaining -= elapsedTime;//保存までの残り時間を減らす

		bool isFinished = Ball::Instance().GetHasCollidedWithFence() || Ball::Instance().GetHasCollidedWithGround();

		bool isHomeRun = (Ball::Instance().GetHasPassedHomeRunZone() && isFinished) || Ball::Instance().GetHasCollidedWithPole();

		//普通のヒット判定
		bool isHit = isFinished && !Ball::Instance().GetIsFoulConfirmed();

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
			//ヒットの時に最高飛距離を更新しても、ホームランをすでに打っていれば保存しない
			else if(BallDistance::Instance().HasUpdatedMaxDistance() && HomeRunCount::Instance().GetTotalHomeRunCount() > 0 && isHit)
			{
				replayFrames.clear();//最高飛距離を更新していなければ保存しない
				if(consoleLog)
				{
					consoleLog->push_back(u8"ヒット時に最高飛距離を更新しても、ホームランをすでに打っているため、録画は保存されませんでした");
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
		hasLooped = false;
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

	hasLooped = false;

	playbackTime += elapsedTime * playbackSpeed;//再生時間・再生速度を更新

	const float lastTime = savedReplayList.back().time;//最後のフレームの時間

	if (playbackTime >= lastTime)
	{
		if (isLoopPlayback)
		{
			playbackTime = 0.0f;
			playbackIndex = 0;
			hasLooped = true;
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
	if(frame1.pitcherCurrentAnimationIndex == frame2.pitcherCurrentAnimationIndex)
	{
		//アニメーションインデックスが同じ場合は、アニメーションタイムを補間
		result.pitcherCurrentAnimationIndex = frame1.pitcherCurrentAnimationIndex;
		result.pitcherAnimationTime = frame1.pitcherAnimationTime + (frame2.pitcherAnimationTime - frame1.pitcherAnimationTime) * t;
	}
	else
	{
		result.pitcherCurrentAnimationIndex = (t < 0.5f) ? frame1.pitcherCurrentAnimationIndex : frame2.pitcherCurrentAnimationIndex;
		result.pitcherAnimationTime = (t < 0.5f) ? frame1.pitcherAnimationTime : frame2.pitcherAnimationTime;
	}

	XMStoreFloat3(&result.batterPosition, XMVectorLerp(XMLoadFloat3(&frame1.batterPosition), XMLoadFloat3(&frame2.batterPosition), t));
	XMStoreFloat3(&result.batterRotation, XMVectorLerp(XMLoadFloat3(&frame1.batterRotation), XMLoadFloat3(&frame2.batterRotation), t));
	if(frame1.batterCurrentAnimationIndex == frame2.batterCurrentAnimationIndex)
	{//アニメーションインデックスが同じ場合は、アニメーションタイムを補間
		result.batterCurrentAnimationIndex = frame1.batterCurrentAnimationIndex;
		result.batterAnimationTime = frame1.batterAnimationTime + (frame2.batterAnimationTime - frame1.batterAnimationTime) * t;
	}
	else
	{
		result.batterCurrentAnimationIndex = (t < 0.5f) ? frame1.batterCurrentAnimationIndex : frame2.batterCurrentAnimationIndex;
		result.batterAnimationTime = (t < 0.5f) ? frame1.batterAnimationTime : frame2.batterAnimationTime;
	}

	result.hasCollidedWithBat = (t < 0.5f) ? frame1.hasCollidedWithBat : frame2.hasCollidedWithBat;

	XMStoreFloat3(&result.ballPosition, XMVectorLerp(XMLoadFloat3(&frame1.ballPosition), XMLoadFloat3(&frame2.ballPosition), t));
	XMStoreFloat3(&result.ballVelocity, XMVectorLerp(XMLoadFloat3(&frame1.ballVelocity), XMLoadFloat3(&frame2.ballVelocity), t));
	XMStoreFloat4(&result.ballRotation, XMQuaternionSlerp(XMLoadFloat4(&frame1.ballRotation), XMLoadFloat4(&frame2.ballRotation), t));

	XMStoreFloat3(&result.cameraEyePosition, XMVectorLerp(XMLoadFloat3(&frame1.cameraEyePosition), XMLoadFloat3(&frame2.cameraEyePosition), t));
	XMStoreFloat3(&result.cameraFocusPosition, XMVectorLerp(XMLoadFloat3(&frame1.cameraFocusPosition), XMLoadFloat3(&frame2.cameraFocusPosition), t));

	return result;
}

void ReplayManager::DrawGUI()
{
	if (ImGui::CollapsingHeader("Replay Manager"))
	{
		ImGui::Text("Recording: %s", isRecording ? "Yes" : "No");
		ImGui::Text("Pending Save: %s", isPendingSave ? "Yes" : "No");
		ImGui::Text("Save Time Remaining: %.2f", saveTimeRemaining);
		ImGui::Text("Saved Replay Frames: %zu", savedReplayList.size());
		ImGui::Text("Playback Time: %.2f", playbackTime);
		ImGui::Text("Is Playing: %s", isPlaying ? "Yes" : "No");
		ImGui::Text("Loop Playback: %s", isLoopPlayback ? "Yes" : "No");

		//バットに当たっているかを表示
		if (isPlaying && !savedReplayList.empty())
		{
			ImGui::Text("Has Collided With Bat: %s", currentPlaybackFrame.hasCollidedWithBat ? "Yes" : "No");
		}
		else
		{
			ImGui::Text("Has Collided With Bat: N/A");
		}
	}
}