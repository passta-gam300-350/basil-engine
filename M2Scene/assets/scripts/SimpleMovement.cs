using BasilEngine;
using BasilEngine.Components;
using BasilEngine.Mathematics;
using BasilEngine.Debug;




public class SimpleMovement : Behavior
{
    public int multiplier = 2;
    private int frame = 165;


    public void Init()
    {

    }
    public void Update()
    {


        if (Input.GetKey(KeyCode.W))
        {
            transform.position = transform.position + new Vector3(0, 0, -(1.0f / frame) * multiplier);
        }
        else if (Input.GetKey(KeyCode.S))
        {
            transform.position = transform.position + new Vector3(0, 0, (1f / frame) * multiplier);
        }
        else if (Input.GetKey(KeyCode.A))
        {
            transform.position = transform.position + new Vector3((-1f / frame) * multiplier, 0, 0);
        }
        else if (Input.GetKey(KeyCode.D))
        {
            transform.position = transform.position + new Vector3((1f / frame) * multiplier, 0, 0);
        }
        else if (Input.GetKey(KeyCode.Q))
        {
            transform.position = transform.position + new Vector3(0, (1f / frame) * multiplier, 0);
        }
        else if (Input.GetKey(KeyCode.E))
        {
            transform.position = transform.position + new Vector3(0, -(1f / frame) * multiplier, 0);
        }
    }

    public void FixedUpdate()
    {

    }
    


}