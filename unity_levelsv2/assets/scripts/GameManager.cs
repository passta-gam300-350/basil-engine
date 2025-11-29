using System.Linq.Expressions;
using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;
using BasilEngine.SceneManagement;


public class GameManager : Behavior
{
    

    public static GameManager instance;
    public bool TrainPuzzleCompleted = false;
    public bool KitePuzzleCompleted = false;
    public bool isCleaned = false;



    private GameObject player;
    private Rigidbody playerRigidbody;
    private GameObject ThrashCollect;

    public bool isHoldingThrash = false;

    private PlayerController3D controller;

    
    public void Init()
    {
        instance = this;

        ThrashCollect = GameObject.Find("ThrashCollect");
        player = GameObject.Find("PlayerGroup");
        playerRigidbody = player.transform.GetComponent<Rigidbody>();
        controller = player.transform.GetComponent<PlayerController3D>();




    }
    public void Update()
    {

        if (isCleaned && KitePuzzleCompleted && TrainPuzzleCompleted)
        {
            Logger.Log("Congratz: Move to Kite Level!");
            Scene.LoadScene(2);
            return;
        }
    }

    public void FixedUpdate()
    {

    }

    public void ShowCollected()
    {
        ThrashCollect.visibility = true;
        isHoldingThrash = true;
    }

    public void ShowFree()
    {
        ThrashCollect.visibility = false;
        isHoldingThrash = false;
    }


    public void DisableControls()
    {
        controller.disabled = true;
        playerRigidbody.FreezeX = true;
        playerRigidbody.FreezeZ = true;


    }
    public void EnableControls()
    {
        controller.disabled = false;
        playerRigidbody.FreezeX = false;
        playerRigidbody.FreezeZ = false;
    }

    public void CompleteTrainPuzzle()
    {
        TrainPuzzleCompleted = true;
    }

    public void CompleteKitePuzzle()
    {
                KitePuzzleCompleted = true;
    }
}