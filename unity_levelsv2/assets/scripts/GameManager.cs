using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;




public class GameManager : Behavior
{
    

    public static GameManager instance;


    private GameObject player;
    private GameObject ThrashCollect;


    private PlayerController3D controller;

    
    public void Init()
    {
        instance = this;

        ThrashCollect = GameObject.Find("ThrashCollect");
        player = GameObject.Find("PlayerGroup");

        controller = player.transform.GetComponent<PlayerController3D>();




    }
    public void Update()
    {
        

    }

    public void FixedUpdate()
    {

    }

    public void ShowCollected()
    {
        
    }


    public void DisableControls()
    {
        controller.disabled = true;
    }
    public void EnableControls()
    {
        controller.disabled = false;
    }
}