uniform sampler2D texUnit1, texUnit2, texUnit3;
uniform float brightness;
uniform float gamma;
uniform float contrast;

void main(void)
{
    vec3 yuv;
    yuv.x = (texture2D(texUnit1, gl_TexCoord[0].st).x + brightness);
    yuv.y = texture2D(texUnit2, gl_TexCoord[1].st).x;
    yuv.z = texture2D(texUnit3, gl_TexCoord[1].st).x;

    gl_FragColor.x = 1.164 * (yuv.x - 16.0/256.0) + 1.596 * (yuv.z - 128.0/256.0);
    gl_FragColor.y = 1.164 * (yuv.x - 16.0/256.0) - 0.813 * (yuv.z - 128.0/256.0) - 0.391 * (yuv.y - 128.0/256.0);
    gl_FragColor.z = 1.164 * (yuv.x - 16.0/256.0)                                 + 2.018 * (yuv.y - 128.0/256.0);
    
    gl_FragColor.xyz = (gl_FragColor.xyz - vec3(0.5, 0.5, 0.5)) * contrast + vec3(0.5, 0.5, 0.5);

    gl_FragColor.x = pow(gl_FragColor.x, gamma);
    gl_FragColor.y = pow(gl_FragColor.y, gamma);
    gl_FragColor.z = pow(gl_FragColor.z, gamma);
}
