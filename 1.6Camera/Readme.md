#Camera
OpenGL本身没有摄像机(Camera)的概念，但我们可以通过把场景中的所有物体往相反方向移动的方式来模拟出摄像机，产生一种我们在移动的感觉，而不是场景在移动。<br>
当我们讨论摄像机/观察空间(Camera/View Space)的时候，是在讨论以摄像机的视角作为场景原点时场景中所有的顶点坐标：观察矩阵把所有的世界坐标变换为相对于摄像机位置与方向的观察坐标。也就是说要创建一个以相机为原点的坐标系。格拉姆-施密特正交化方法。

#视角移动
对于我们的摄像机系统来说，我们只关心俯仰角(pitch)和偏航角(yaw)
![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/1.6Camera/Reference/camera_pitch_yaw_roll.png)

#LookAt矩阵
如果你使用3个相互垂直（或非线性）的轴定义了一个坐标空间，你可以用这3个轴外加一个平移向量来创建一个矩阵，并且你可以用这个矩阵乘以任何向量来将其变换到那个坐标空间。这正是LookAt矩阵所做的。我们要做的只是定义一个摄像机位置，一个目标位置和一个表示世界空间中的上向量的向量。
<img src="http://latex.codecogs.com/svg.latex?LookAt=\left[\begin{array}{cccc}{R_{x}}&{R_{y}}&{R_{z}}&{0}\\{U_{x}}&{U_{y}}&{U_{z}}&{0}\\{D_{x}}&{D_{y}}&{D_{z}}&{0}\\{0}&{0}&{0}&{1}\end{array}\right]*\left[\begin{array}{cccc}{1}&{0}&{0}&{-P_{x}}\\{0}&{1}&{0}&{-P_{y}}\\{0}&{0}&{1}&{-P_{z}}\\{0}&{0}&{0}&{1}\end{array}\right]" />