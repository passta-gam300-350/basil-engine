using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using BasilEngine.SceneManagement;


public class PlayAudioLoop : Behavior
{

    private Audio audio;
    private float fadeSpeed = 100.0f; // 100 = 1 second, 50 = 2 second...
    private bool isFadingIn = false;
    private float targetVolume;

    public void Init()
    {
        audio = transform.GetComponent<Audio>();
        targetVolume = audio.Volume;
        audio.Volume = 0;
        audio.Looping = true;
        audio.Play();
        isFadingIn = true;
    }
    public void Update()
    {
        if (isFadingIn)
        {
            audio.Volume += (targetVolume * fadeSpeed / 100.0f) * Time.deltaTime;

            if (audio.Volume >= targetVolume)
            {
                audio.Volume = targetVolume;
                isFadingIn = false;
            }
        }
    }

    public void FixedUpdate()
    {

    }


}