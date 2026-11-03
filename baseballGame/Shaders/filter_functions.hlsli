#ifndef __FILTER_FUNCTIONS_HLSLI__
#define __FILTER_FUNCTIONS_HLSLI__

//--------------------------------------------
//	RGB色空間の数値からHSV色空間の数値への変換関数
//--------------------------------------------
//rgb:RGB色空間の数値
float3 convert_rgb_to_hsv(float3 rgb)
{
    float3 hsv = 0;
    
    float Vmax = max(rgb.r, max(rgb.g, rgb.b)); // RGBの三つの値で最大のもの
    float Vmin = min(rgb.r, min(rgb.g, rgb.b)); // RGBの三つの値で最小のもの
    float delta = Vmax - Vmin; // RGBの三つの値の差
    
    //V : 明度
    hsv.z = Vmax;
    
    //S : 彩度
    hsv.y = (delta / Vmax) * step(0, Vmax); //Vmaxが0の時はS=0とする
    
    //H : 色相
    if(hsv.y > 0.0f)
    {
        if(rgb.r == Vmax)
        {
            hsv.x = 60 * ((rgb.g - rgb.b) / delta); //Rが最大の時
        }
        else if (rgb.g == Vmax)
        {
            hsv.x = 60 * ((rgb.b - rgb.r) / delta) + 120; //Gが最大の時
        }
        else
        {
            hsv.x = 60 * ((rgb.r - rgb.g) / delta) + 240; //Bが最大の時
        }
        
        if(hsv.x < 0.0f)
        {
            hsv.x += 360; //Hが負の値になった場合は360を足す
        }
    }
    
    return hsv;

}

//--------------------------------------------
//	HSV色空間の数値からRGB色空間の数値への変換関数
//--------------------------------------------
//hsv:HSV色空間の数値
float3 convert_hsv_to_rgb(float3 hsv)
{
    float3 rgb = 0;
    if(hsv.y == 0)
    {
        rgb.r = rgb.g = rgb.b = hsv.z; //Sが0の時はR=G=B=Vとする
    }
    else
    {
        // 色環のH（色相）の位置とS（彩度）、V（明度）からRGB値を算出する
        float Vmax = hsv.z;
        float Vmin = Vmax - (hsv.y * Vmax);
        
        hsv.x %= 360; //Hが360以上の値になった場合は360で割った余りを使用する
        
        float Huei = (int) (hsv.x / 60); //Hを60で割った商を算出する
        float Huef = (hsv.x / 60) - Huei; //Hを60で割った余りを算出する
        float p = Vmax * (1 - hsv.y);
        float q = Vmax * (1 - (hsv.y * Huef));
        float t = Vmax * (1 - (hsv.y * (1 - Huef)));
        if (Huei == 0)
        {
            rgb.r = Vmax; //Rが最大の時
            rgb.g = t;
            rgb.b = p;
        }
        else if (Huei == 1)
        {
            rgb.r = q;
            rgb.g = Vmax; //Gが最大の時
            rgb.b = p;
        }
        else if (Huei == 2)
        {
            rgb.r = p;
            rgb.g = Vmax; //Bが最大の時
            rgb.b = t;
        }
        else if (Huei == 3)
        {
            rgb.r = p;
            rgb.g = q;
            rgb.b = Vmax; //Bが最大の時
        }
        else if (Huei == 4)
        {
            rgb.r = t;
            rgb.g = p;
            rgb.b = Vmax; //Bが最大の時
        }
        else if (Huei == 5)
        {
            rgb.r = Vmax; //Rが最大の時
            rgb.g = p;
            rgb.b = q;
        }

    }
    return rgb;
}

//--------------------------------------------
//	RGB色から輝度への変換関数
//--------------------------------------------
float convert_rgb_to_luminance(float3 rgb)
{
    static const float3 luminanceValue = float3(0.299f, 0.587f, 0.114f);
    return dot(luminanceValue, rgb);
}

#endif	//	__FILTER_FUNCTIONS_HLSLI__
