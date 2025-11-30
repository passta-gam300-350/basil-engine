using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using BasilEngine.SceneManagement;
using System.Runtime.InteropServices;


public class Spool : Behavior
{

    private Audio audio;
    public float rotationSpeed = 1.0f;

    public void Init()
    {
        audio = transform.GetComponent<Audio>();
    }
    public void Update()
    {
        if (Input.GetKey(KeyCode.A))
        {
            audio.Stop();
            gameObject.transform.rotation = new Vector3(0f, 0f, 30f);
            audio.Play();
        }
        if (Input.GetKey(KeyCode.D))
        {
            audio.Stop();
            gameObject.transform.rotation = new Vector3(0f, 0f, -30f);
            audio.Play();
        }
    }

    public void FixedUpdate()
    {

    }


}