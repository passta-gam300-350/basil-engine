using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using BasilEngine.SceneManagement;


public class MainMenu : Behavior
{

    private Audio audio;
    private float fadeSpeed = 100.0f;
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

        Logger.Log("Main Menu");

        if (isFadingIn)
        {
            audio.Volume += (targetVolume * fadeSpeed / 100.0f) * Time.deltaTime;
            
            if (audio.Volume >= targetVolume)
            {
                audio.Volume = targetVolume;
                isFadingIn = false;
            }
        }
        
        if (Input.GetKey(KeyCode.ENTER))
        {
            // Load the main game scene
            Logger.Warn("TODO: Scene loading");
            Scene.LoadScene(1);
        }

    }

    public void FixedUpdate()
    {

    }


}