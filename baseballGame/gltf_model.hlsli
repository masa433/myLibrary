// 頂点シェーダーへの入力構造体
struct VS_IN
{
    float4 position : POSITION; // 頂点位置
    float4 normal : NORMAL; // 法線ベクトル
    float4 tangent : TANGENT; // 接線ベクトル
    float2 texcoord : TEXCOORD; // テクスチャ座標
    uint4 joints : JOINTS; // スキニング用のジョイントインデックス
    float4 weights : WEIGHTS; // スキニング用のウェイト
};

// 頂点シェーダーからピクセルシェーダーへの出力構造体
struct VS_OUT
{
    float4 position : SV_POSITION; // クリップ空間での頂点位置（システム用セマンティクス）
    float4 w_position : POSITION; // ワールド空間での頂点位置
    float4 w_normal : NORMAL; // ワールド空間での法線ベクトル
    float4 w_tangent : TANGENT; // ワールド空間での接線ベクトル
    float2 texcoord : TEXCOORD; // テクスチャ座標
};

// プリミティブごとの定数バッファ（b0）
cbuffer PRIMITIVE_CONSTANT_BUFFER : register(b0)
{
    row_major float4x4 world; // ワールド変換行列
    int material; // マテリアルインデックス
    bool has_tangent; // 接線情報があるかどうか
    int skin; // スキン（ボーンセット）のインデックス
    int pad; // パディング（未使用、アライメント調整用）
};

// シーン全体の定数バッファ（b1）
cbuffer SCENE_CONSTANT_BUFFER : register(b1)
{
    row_major float4x4 view_projection; // ビュー・プロジェクション行列
    float4 light_direction; // ライトの方向ベクトル
    float4 camera_position; // カメラのワールド座標
}

//UNIT.37
static const uint PRIMiTIVE_MAX_JOINTS = 512;//最大ジョイント数
cbuffer PRIMITIVE_JOINT_CONSTANTS : register(b2)
{
    row_major float4x4 joint_matrices[PRIMiTIVE_MAX_JOINTS]; //ジョイント変換行列
};