<h2>平行光</h2>
当我们使用一个假设光源处于无限远处的模型时，它就被称为定向光，因为它的所有光线都有着相同的方向，它与光源的位置是没有关系的。比如太阳光。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/2.5Light%20casters/Reference/light_casters_directional.png)


<h2>点光源</h2>
在大部分的3D模拟中，我们都希望模拟的光源仅照亮光源附近的区域而不是整个场景，所以点光源做好的核心是做好衰减。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/2.5Light%20casters/Reference/light_casters_point.png)

根据片段距光源的距离计算了衰减值，之后我们会将它乘以光的强度向量：

<img src="http://latex.codecogs.com/svg.latex?F_{att}=\frac{1.0}{K_{c}+K_{l}*d+K_{q}*d^{2}}" />

在这里d代表了片段距光源的距离。接下来为了计算衰减值，我们定义3个（可配置的）项：常数项Kc、一次项Kl和二次项Kq。

常数项通常保持为1.0，它的主要作用是保证分母永远不会比1小，否则的话在某些距离上它反而会增加强度，这肯定不是我们想要的效果。
一次项会与距离值相乘，以线性的方式减少强度。
二次项会与距离的平方相乘，让光源以二次递减的方式减少强度。二次项在距离比较小的时候影响会比一次项小很多，但当距离值比较大的时候它就会比一次项更大了。
由于二次项的存在，光线会在大部分时候以线性的方式衰退，直到距离变得足够大，让二次项超过一次项，光的强度会以更快的速度下降。这样的结果就是，光在近距离时亮度很高，但随着距离变远亮度迅速降低，最后会以更慢的速度减少亮度。<br>
参考值：

|距离|常数项|一次项|二次项|
|:-:|:-:|:-:|:-:|
|7|1.0|0.7|1.8|
|13|1.0|0.35|0.44|
|20|1.0|0.22|0.20|
|32|1.0|0.14|0.07|
|50|1.0|0.09|0.032|
|65|1.0|0.07|0.017|
|100|1.0|0.045|0.0075|
|160|1.0|0.027|0.0028|
|200|1.0|0.022|0.0019|
|325|1.0|0.014|0.0007|
|600|1.0|0.007|0.0002|
|3250|1.0|0.0014|0.000007|

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/2.5Light%20casters/Reference/attenuation.png)

光在近距离的时候有着最高的强度，但随着距离增长，它的强度明显减弱，并缓慢地在距离大约100的时候强度接近0。

<h2>聚光</h2>
聚光是位于环境中某个位置的光源，它只朝一个特定方向而不是所有方向照射光线。这样的结果就是只有在聚光方向的特定半径内的物体才会被照亮，其它的物体都会保持黑暗。聚光很好的例子就是路灯或手电筒。

OpenGL中聚光是用一个世界空间位置、一个方向和一个切光角(Cutoff Angle)来表示的，切光角指定了聚光的圆锥半径。对于每个片段，我们会计算片段是否位于聚光的切光方向之间（也就是在锥形内），如果是的话，我们就会相应地照亮片段。

![image](https://github.com/yu-cao/OpenGL-Learning/blob/master/2.5Light%20casters/Reference/light_casters_spotlight_angles.png)

+ `LightDir`：从片段指向光源的向量。
+ `SpotDir`：聚光所指向的方向。
+ `ϕ`：指定了聚光半径的切光角。落在这个角度之外的物体都不会被这个聚光所照亮。
+ `θ`：LightDir向量和SpotDir向量之间的夹角。在聚光内部的话θ值应该比ϕ值小。

我们要做的就是计算LightDir向量和SpotDir向量之间的点积，并将它与切光角ϕ值对比。
	<h3>平滑聚光边缘</h3>
	为了创建一种看起来边缘平滑的聚光，我们需要模拟聚光有一个内圆锥(Inner Cone)和一个外圆锥(Outer Cone)。我们可以将内圆锥设置为上一部分中的那个圆锥，但我们也需要一个外圆锥，来让光从内圆锥逐渐减暗，直到外圆锥的边界。<br>
	为了创建一个外圆锥，我们只需要再定义一个余弦值来代表聚光方向向量和外圆锥向量（等于它的半径）的夹角。然后，如果一个片段处于内外圆锥之间，将会给它计算出一个0.0到1.0之间的强度值。如果片段在内圆锥之内，它的强度就是1.0，如果在外圆锥之外强度值就是0.0。

我们可以用下面这个公式来计算这个值：

<img src="http://latex.codecogs.com/svg.latex?I=\frac{\theta-\gamma}{\epsilon}" />

我们基本是在内外余弦值之间根据θ插值（上述运算均在弧度制下完成）。
