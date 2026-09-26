#pragma once
#include <vector>
#include <memory>
#include <DirectXMath.h>
#include <string>
#include <Ball.h>

//1フレーム分のリプレイデータを格納する構造体
struct ReplayFrame
{
	float time; //投球開始からの経過時間

	DirectX::XMFLOAT3 pitcherPosition; //ピッチャーの位置
	DirectX::XMFLOAT3 pitcherRotation; //ピッチャーの回転（クォータニオン）
	int pitcherCurrentAnimationIndex; //ピッチャーのアニメーションインデックス
	float pitcherAnimationTime; //ピッチャーのアニメーションタイム

	DirectX::XMFLOAT3 batterPosition; //バッターの位置
	DirectX::XMFLOAT3 batterRotation; //バッターの回転（クォータニオン）
	int batterCurrentAnimationIndex; // バッターのアニメーションインデックス
	float batterAnimationTime; //バッターのアニメーションタイム
	bool hasCollidedWithBat; //ボールがバットに当たったかどうか

	//ボールの位置
	DirectX::XMFLOAT3 ballPosition; //ボールの位置
	DirectX::XMFLOAT3 ballVelocity; //ボールの速度
	DirectX::XMFLOAT4 ballRotation; //ボールの回転（クォータニオン）

	//カメラ
	DirectX::XMFLOAT3 cameraEyePosition; //カメラの位置
	DirectX::XMFLOAT3 cameraFocusPosition; //カメラの注視点

};

class ReplayManager
{

public:

	static ReplayManager& Instance()
	{
		static ReplayManager instance;
		return instance;
	}

	ReplayManager() = default;
	~ReplayManager() = default;

	ReplayManager(const ReplayManager&) = delete;// コピーコンストラクタを削除
	ReplayManager& operator=(const ReplayManager&) = delete;// コピー代入演算子を削除
	void Initialize();
	void Uninitialize();

	void DrawGUI(); //GUI描画関数

	//ピッチャーが投げ始めたときの記録開始関数
	void StartRecording(float startTime);

	//1フレーム分のリプレイデータを記録する関数
	void RecordFrame(const ReplayFrame& frame, float elapsedTime);

	//ホームラン着弾時に呼び出す関数
	void OnHomeRunHit();

	void OnHit(); //ホームラン以外のヒット時に呼び出す関数

	//リプレイデータを保存する関数
	void SaveRecording(float elapsedTime);

	//記録をクリアする関数
	void ClearRecording();

	void ClearSavedReplayList() { savedReplayList.clear(); } //保存済みのリプレイデータをクリアする関数

	bool IsRecording() const { return isRecording; } //記録中かどうかを返す関数
	bool IsPendingSave() const { return isPendingSave; } //保存待機中かどうかを返す関数
	float GetSaveTimeRemaining() const { return saveTimeRemaining; } //保存までの残り時間を返す関数

	//保存済みのリプレイデータを取得する関数
	const std::vector<ReplayFrame>& GetSavedReplayList() const { return savedReplayList; }
	float GetRecordingStartTime() const { return recordingStartTime; } //記録開始時間を返す関数

public:

	//映像を再生する関数
	void StartPlayback();

	//再生を更新する関数
	void UpdatePlayback(float elapsedTime);

	//現在の再生時間における補間済みフレームを取得する関数
	const ReplayFrame& GetCurrentPlaybackFrame() const { return currentPlaybackFrame; }

	bool IsPlaying() const { return isPlaying; } //再生中かどうか
	bool HasSavedReplay() const { return !savedReplayList.empty(); } //再生できるリプレイがあるかどうか

	void SetLoopPlayback(bool loop) { isLoopPlayback = loop; } //ループ再生するかどうかを設定
	void StopPlayback() { isPlaying = false; } //再生を停止する関数

	//ループ再生したかどうか
	bool HasLoopedPlayback() const { return hasLooped; }

	// 現在の再生場所が最初かどうか
	bool IsAtFirstFrame() const { return playbackIndex == 0 && playbackTime <= 0.05f; }

	// 現在の再生場所が最後かどうか
	bool IsAtLastFrame() const
	{
		if (savedReplayList.empty()) return false;
		return playbackTime >= savedReplayList.back().time;
	}

	void SetPlaybackSpeed(float speed) { playbackSpeed = speed; } //再生速度を設定する関数
	float GetPlaybackSpeed() const { return playbackSpeed; } //再生速度を取得する関数

private:
	std::vector<ReplayFrame> replayFrames; //リプレイデータを格納するベクター
	std::vector<ReplayFrame> savedReplayList;//保存済みのリプレイデータを格納するベクター

	float recordingStartTime = 0.0f; //記録開始時間
	float homeRunHitTime = 0.0f; //ホームラン着弾時間
	float saveTimeRemaining = 0.0f; //保存までの残り時間

	bool isRecording = false; //記録中かどうかのフラグ
	bool isPendingSave = false; //保存待機中かどうかのフラグ

	float recordElapsedTime = 0.0f; //記録中の経過時間

	bool hasLooped = false;//ループ再生したかどうかのフラグ

	float playbackSpeed = 1.0f; //再生速度（1.0fが通常速度）
private:

	//2つのフレーム間を補間する関数
	ReplayFrame LerpFrame(const ReplayFrame& frame1, const ReplayFrame& frame2, float t);

	float playbackTime = 0.0f; //再生中の経過時間
	size_t playbackIndex = 0; //再生中のフレームインデックス
	ReplayFrame currentPlaybackFrame; //現在の再生時間における補間済みフレーム
	bool isPlaying = false; //再生中かどうかのフラグ
	bool isLoopPlayback = false; //ループ再生するかどうかのフラグ

public:
	// コンソールログへのポインタをセット
	void SetConsoleLog(std::vector<std::string>* log) { consoleLog = log; }

private:
	std::vector<std::string>* consoleLog = nullptr;


};