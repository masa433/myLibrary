#pragma once

#include <xaudio2.h>
#include <x3daudio.h>
#include <mmreg.h>
#include <thread>
#include <atomic>
#include <mutex>

using namespace DirectX;

/**
 * @brief 3Dオーディオシステムを管理するクラス
 *
 * XAudio2とX3DAudioを用いて複数の音源（エミッター）を3D空間上に配置し、
 * リスナーの位置・向きに応じて立体音響を実現する。
 */
class Audio3DSystem
{
public:
	static Audio3DSystem& Instance()
	{
		static Audio3DSystem instance;
		return instance;
	}
	/**
	 * @brief オーディオシステムの初期化を行う
	 *
	 * XAudio2エンジンの初期化、マスタリングボイスの生成、X3DAudioの初期化を行う。
	 * 出力デバイスのチャンネル数（ステレオなど）も取得し内部保持する。
	 */
	void Initialize();


	float bgmVolume = 1;
	float seVolume = 1;
	float masterVolume = 1;
	enum class SoundType {
		BGM,
		SE,
		cnt,
	};
	enum class FadeState {
		None,
		FadeIn,
		FadeOut
	};

	struct FadeInfo {
		FadeState state = FadeState::None;
		float timer = 0.0f;
		float duration = 1.0f;
		float startVolume = 0.0f;
		float targetVolume = 1.0f;
	};
	/**
	 * @brief エミッター（音源）を追加する
	 *
	 * 指定されたWAVファイルを読み込み、指定位置に3D音源として追加する。
	 * ループ再生設定や、無指向性設定、距離減衰なし（BGM）も指定可能。
	 *
	 * @param wavPath 音声ファイルのパス（WAV形式）
	 * @param pos 3D空間上の音源の位置（XMFLOAT3）
	 * @param tag エミッター識別用のタグ文字列
	 * @param loop ループ再生するかどうかのフラグ（true:ループ再生）
	 * @param isOmnidirectional 無指向性音源として再生するかどうか
	 * @param constantVolume 距離減衰を無効にするか（BGM等に使用）
	 * @param distanceScaler 距離減衰のスケーリング値（デフォルトは1.0f）
	 */
	void AddEmitter(const char* wavPath, const XMFLOAT3& pos, const std::string& tag, SoundType soundType,
		float volume, bool loop, bool isOmnidirectional = false, bool constantVolume = false, float distanceScaler = 1.0f);

	/**
	 * @brief リスナーの位置と向きを更新する
	 *
	 * 3D空間内のリスナー（聴き手）の位置と向きベクトルを設定し、
	 * それに基づき3D音響の計算を行う準備をする。
	 *
	 * @param pos リスナーの3D位置
	 * @param front リスナーの正面方向ベクトル
	 * @param up リスナーの上方向ベクトル
	 */
	void UpdateListener(const XMFLOAT3& pos, const XMFLOAT3& front, const XMFLOAT3& up);

	/**
	 * @brief すべてのエミッターの3D音響処理を更新する
	 *
	 * 各エミッターの位置に基づき、リスナーからの距離や方向を計算し、
	 * ボリュームやパン、音源の3D音響パラメータをXAudio2のSourceVoiceに反映させる。
	 */
	void UpdateEmitters(float elapsedTime);

	/**
	 * @brief 指定したタグを持つエミッターの位置を変更する
	 *
	 * 登録済みのエミッターの中から、指定タグと一致するものを探し、
	 * その3D位置を新しい位置に更新する。
	 *
	 * @param tag 変更対象のエミッターを識別するタグ文字列
	 * @param newPos 新しい3D位置
	 */
	void SetEmitterPositionByTag(const std::string& tag, const XMFLOAT3& newPos);

	/**
	 * @brief WAVファイルを読み込み、フォーマット情報と再生バッファをセットアップする
	 *
	 * 指定されたWAVファイルから音声データを読み込み、
	 * WAVEFORMATEX構造体、XAUDIO2_BUFFER構造体、音声データバッファを設定する。
	 *
	 * @param filename 読み込むWAVファイルのパス
	 * @param outFormat WAVのフォーマット情報を格納する構造体のポインタ
	 * @param outBuffer XAUDIO2_BUFFER構造体のポインタ（音声データと再生情報）
	 * @param waveData 音声データを格納するバイト配列（呼び出し元で管理される）
	 * @return 読み込み成功ならtrue、失敗ならfalse
	 * @see https://learn.microsoft.com/ja-jp/windows/win32/api/mmreg/ns-mmreg-waveformatex
	 */
	bool LoadWavFile(const char* filename, WAVEFORMATEX* outFormat, XAUDIO2_BUFFER* outBuffer, std::vector<BYTE>& waveData);

	/**
	 * @brief すべてのエミッターの音声を再生開始する
	 */
	void PlayAll();

	/**
	 * @brief 指定タグを持つすべてのエミッターの音声を再生開始する
	 *
	 * @param tag 再生対象のエミッターを識別するタグ文字列
	 */
	void PlayByTag(const std::string& tag);

	/**
	 * @brief 指定タグを持つすべてのエミッターの音声を停止する
	 *
	 * @param tag 停止対象のエミッターを識別するタグ文字列
	 */
	void StopByTag(const std::string& tag);

	/**
	* @brief 指定タグのエミッターのみ音量を変更する
	*
	* @param tag 音量変更対象のタグ文字列
	* @param volume 音量(0~1)
	*/
	void SetVolumeByTag(const std::string& tag, float volume);
	void SetVolumeByAll();

	///**
	// * @brief エミッター更新用スレッドの開始
	// */
	//void StartUpdateThread(float elapsedTime);
	//
	///**
	// * @brief エミッター更新用スレッドの停止
	// */
	//void StopUpdateThread();
	void StartFadeIn(const std::string& tag, float duration);

	void StartFadeOut(const std::string& tag, float duration);
	void UpdateFadeVolumes(float elapsedTime);


private:
	IXAudio2* xAudio2 = nullptr; ///< XAudio2インスタンスへのポインタ
	IXAudio2MasteringVoice* masteringVoice = nullptr; ///< マスタリングボイスへのポインタ（最終出力）

	X3DAUDIO_HANDLE x3dAudioHandle; ///< X3DAudioのハンドル（初期化時に作成）

	UINT32 outputChannelCount = 2; ///< 出力デバイスのチャンネル数（例：ステレオなら2）

	X3DAUDIO_LISTENER listener; ///< リスナーの3D空間上の情報
	std::thread updateThread; ///< エミッターの更新処理を行うバックグラウンドスレッド
	std::atomic<bool> running = false; ///< スレッド実行中フラグ
	std::mutex dataMutex; ///< リスナーとエミッターへのアクセス保護用ミューテックス

	/**
	 * @brief 音源を表す構造体（エミッター）
	 */
	struct Emitter {
		X3DAUDIO_EMITTER emitterDesc; ///< X3DAudioによる音響処理に必要な情報構造体
		IXAudio2SourceVoice* sourceVoice; ///< 個々の音声再生を行うソースボイス
		WAVEFORMATEX waveFormat; ///< WAVファイルのフォーマット情報
		XAUDIO2_BUFFER buffer; ///< 音声データと再生情報を格納する構造体
		std::vector<BYTE> waveData; ///< WAVファイルの実データ（バイト列）
		XMFLOAT3 position; ///< 3D空間上の音源の位置
		std::string tag; ///< エミッター識別用の文字列タグ
		SoundType soundType = SoundType::SE;
		bool loop = false; ///< ループ再生フラグ
		bool isOmnidirectional = false; ///< 無指向性音源フラグ（trueなら方向に関係なく音が聞こえる）
		bool constantVolume = false; ///< 距離減衰を無効にするフラグ（BGMなどに使用）
		float distanceScaler = 1.0f; ///< 距離減衰スケーラー（距離減衰なしなら0.0f）
		float Volume;
		FadeInfo fadeInfo;
		float minDistance; // 追加
		float maxDistance; // 追加
	};

	std::vector<Emitter> emitters; ///< 登録済みのすべてのエミッターのリスト

public:
	const std::vector<Emitter>& GetEmitters() const { return emitters; }

	template<typename T>
	T Lerp(T a, T b, T t)
	{
		return a + (b - a) * t;
	}
};